#include "console.h"
#include "linux/errno.h"
#include "linux/fcntl.h"
#include "file.h"
#include "mm.h"
#include "kmalloc.h"
#include "log.h"
#include "memlayout.h"
#include "mman.h"
#include "mmap.h"
#include "mmu.h"
#include "pagecache.h"
#include "proc.h"
#include "spinlock.h"
#include "string.h"
#include "types.h"
#include "vm.h"

/* utils */

/*
 * struct mmap_regionリンクリストからnodeを削除して、node->nextをprevに
 * つなぐ。munmap()が呼ばれた時に呼び出される
 */
static void
delete_mmap_node(struct proc *p, struct mmap_region *node)
{
    if (p->regions == 0) return;

    //cprintf("delete_mmap_node[%d]: addr=0x%p, original=%d\n", p->pid, node->addr, node->original);

    //print_mmap_list(p, "delete node before");

    struct mmap_region *region, *prev;
    if (node->addr == p->regions->addr) {
        if (p->regions->next != 0)
            p->regions = p->regions->next;
        else
            p->regions = 0;
    } else {
        region = prev = p->regions;
        while (region) {
            if (node->addr == region->addr) {
                if (region->next != 0)
                    prev->next = region->next;
                else
                    prev->next = 0;
                break;
            }
            prev = region;
            region = region->next;
        }
    }
    // FIXME: paをshareしているmmapがすべて削除されたことを判断する方法
    int free = (node->flags & MAP_SHARED) ? 0 : 1;
    uvm_unmap(p->pgdir, (uint64_t)node->addr, (((uint64_t)node->length + PGSIZE - 1) / PGSIZE), free);
    kmfree(node);
    node = 0;
    //print_mmap_list(p, "delete node after");
}

/*
 * struct mmap_regionリンクリスト全体をクリアする。
 * ユーザ空間にあるメモリページをすべて開放しなければ
 * ならない時にexecから呼び出される
 */
void
free_mmap_list(struct proc *p)
{
    struct mmap_region* region = p->regions;
    struct mmap_region* temp;

    while (region) {
        temp = region;
        //if (!(region->flags & MAP_SHARED)) {
            if (region->f) {
                fileclose(region->f);
            }
            delete_mmap_node(p, region);
            p->nregions -= 1;
        //}
        region = temp->next;
    }
}

// srcからdestにmmap_regionをコピー
void
copy_mmap_region(struct mmap_region *dest, struct mmap_region *src)
{
    dest->addr          = src->addr;
    dest->length        = src->length;
    dest->flags         = src->flags;
    dest->prot          = src->prot;
    dest->offset        = src->offset;
    dest->next          = NULL;
    dest->original      = 0;

    if (!(src->flags & MAP_ANONYMOUS) && src->f) {
        dest->f = filedup(src->f);
    } else {
        dest->f = NULL;
    }
}

// 指定されたprotectionからpermを作成する
uint64_t
get_perm(int prot, int flags)
{
    uint64_t perm;
    if (flags & MAP_ANONYMOUS)
        perm = PTE_USER | PTE_PXN | PTE_PAGE
             | (MT_NORMAL_NC << 2) | PTE_AF | PTE_SH;
    else
        perm = PTE_USER | PTE_PXN | PTE_PAGE
             | (MT_NORMAL << 2) | PTE_AF | PTE_SH;

    if (prot & PROT_READ)
        perm |= PTE_RO;
    if (prot & PROT_WRITE)
        perm &= ~PTE_RO;
    if (!(prot & PROT_EXEC))
        perm |= PTE_UXN;
    if (prot & PROT_NONE || !(prot & (PROT_READ | PROT_WRITE)))
        perm &= ~PTE_USER;

    return perm;
}

// regionのサイズをsizeにする（拡大する場合は、拡大可能であることが保証されていること）
static long
scale_mmap_region(struct mmap_region *region, uint64_t size)
{
    long error;

    if (region->length == size) return 0;

    if (region->length < size) { // 拡大
        if ((error = mmap_load_pages(region->addr + region->length, size - region->length, region->prot, region->flags, region->f, region->offset)) < 0)
            return error;
    } else {                        // 縮小
        uint64_t dstart = ROUNDDOWN((uint64_t)region->addr + region->length, PGSIZE);
        uint64_t dpages = (region->length - dstart + PGSIZE - 1) / PGSIZE;
        int free = (region->flags & MAP_SHARED) ? 0 : 1;
        if ((uint64_t)region->addr + size <= dstart && dpages > 0) {
            uvm_unmap(thisproc()->pgdir, dstart, dpages, free);
        }
    }
    region->length = size;

    return 0;
}

/*
 * addr, lengthを持つmmap_regionを作成できるか
 * できる: 1, できない: 0
 */
static int
is_usable(void *addr, size_t length)
{
    struct proc *p = thisproc();

    if (p->nregions == 0) return 1;

    struct mmap_region *cursor = p->regions;
    while (cursor) {
        // 1: 右端が最左のregionより小さい
        if (addr + length <= cursor->addr)
            return 1;
        // 2: 左端が最左のregionより大きい、かつ、次のregionがないか、左端が次のregionより小さい
        if (cursor->addr + cursor->length <= addr && (cursor->next == 0 || addr + length <= cursor->next->addr))
            return 1;
        cursor = cursor->next;
    }

    return 0;
}

/*
 *背後にファイルが存在するマッピング
 *  ページキャッシュページをプロセスにマッピングする
 */
static long
map_file_page(void *addr, size_t length, uint64_t perm, struct file *f, off_t offset)
{
    struct proc *p = thisproc();
    long error;

    //cprintf("map_file_page: addr=0x%p, length=0x%llx, offset=0x%llx\n", addr, length, offset);
    char *mem = kalloc();
    if (!mem) {
        return -ENOMEM;
    }
    memset(mem, 0, PGSIZE);
    // ファイルコンテンツをページキャッシュからメモリへコピー
    size_t tempsize = length;
    int i = 0;
    while (tempsize > 0) {
        off_t curoff = offset % PGSIZE;
        size_t cursize = (PGSIZE - curoff) > tempsize ? tempsize : (PGSIZE - curoff);
        //cprintf("- tempsize=0x%x, curoff=0x%x, cursize=0x%x, ip-size=0x%x\n", tempsize, curoff, cursize, f->ip->size);
        if (curoff > f->ip->size) {
            //cprintf("- curoff > ip->size\n");
            break;
        }
        if ((error = copy_page(f->ip, offset + PGSIZE * i, (mem + length - tempsize), cursize, curoff)) < 0) {
            warn("map_pagecache_page: copy_page failed");
            kfree(mem);
            return error;
        }
        tempsize -= cursize;
        offset = 0;
        i += 1;
    }
    // ページをユーザプロセスにマッピング
    //cprintf("- map_region: addr=0x%p, mem=0x%p\n", (void *)addr, mem);
    if ((error = uvm_map(p->pgdir, (void *)addr, PGSIZE, V2P(mem))) < 0) {
        //cprintf("map_pagecache_page: map_region failed\n");
        kfree(mem);
        return error;
    }

    //cprintf("- map_region: addr=0x%llx, mem=0x%x\n", addr, V2P(mem));
    return 0;
}

// ファイルが背後にあるマッピングにファイルからデータをロードする
static long
map_file_pages(void *addr, size_t length, uint64_t perm, struct file *f, off_t offset)
{
    long error;
    size_t mapsize, size = length;
    //cprintf("map_file_pages: addr=%p, length=0x%llx, perm=0x%llx, f=%d\n", addr, length, perm, f->ip->inum);
    for (uint64_t cur = 0; cur < length; cur += PGSIZE) {
        mapsize = PGSIZE > size ? size : PGSIZE;
        if ((error = map_file_page(addr + cur, mapsize, perm, f, offset + cur)) < 0) {
            if (cur != 0)
                uvm_unmap(thisproc()->pgdir, addr, cur/PGSIZE, 1);
            return error;
        }
        size -= mapsize;
    }
    return 0;
}

// 無名マッピングに1ページを割り当てる
static long
map_anon_page(void *addr, uint64_t perm)
{
    struct proc *p = thisproc();

    char *page = kalloc();
    if (!page) {
        //cprintf("map_anon_page: memory exhausted\n");
        return -ENOMEM;
    }
    memset(page, 0, PGSIZE);
    //cprintf("map_anon_page: map addr=0x%llx, page=0x%p\n", addr, V2P(page));
    //if (map_region(p->pgdir, addr, PGSIZE, V2P(page), perm) < 0) {
    if (uvm_map(p->pgdir, addr, PGSIZE, V2P(page)) < 0) {
        kfree(page);
        return -EINVAL;
    }
    return 0;
}

// 無名ページに（複数）ページを割り当てる
static long
map_anon_pages(void *addr, size_t length, uint64_t perm)
{
    long ret;
    //cprintf("map_file_pages: addr=%p, length=0x%llx, perm=0x%llx\n", addr, length, perm);
    for (uint64_t cur = 0; cur < length; cur += PGSIZE) {
        if ((ret = map_anon_page(addr + cur, perm)) < 0) {
            if (cur != 0)
                uvm_unmap(thisproc()->pgdir, addr, cur/PGSIZE, 1);
            return ret;
        }

    }
    return 0;
}

/* addrにメモリを割り当てる */
long
mmap_load_pages(void *addr, size_t length, int prot, int flags, struct file *f, off_t offset)
{
    uint64_t perm = get_perm(prot, flags);
    if ((flags & MAP_PRIVATE) && !(flags & MAP_POPULATE)) perm |= PTE_RO;
    if (flags & MAP_SHARED) perm &= ~PTE_RO;

    if (flags & MAP_ANONYMOUS)
        return map_anon_pages(addr, length, perm);
    else
        return map_file_pages(addr, length, perm, f, offset);
}

// sys_mmapのメイン関数
long
mmap(void *addr, size_t length, int prot, int flags, struct file *f, off_t offset)
{
    struct proc *p = thisproc();
    long error;

    // MAP_FIXEDの指定アドレスはページ境界にあり、割り当て領域がMMAPエリア内に入ること

    // 1. addrを確定する

    // 1.1. アドレスが指定されている場合
    if (addr != 0) {
        // 1.1.1 指定アドレス+指定サイズはUSERTOPを超えないこと
        // upper_addr: addr + sizeがMMAPTOPを超えないかチェックするための変数
        void *upper_addr = (void *)ROUNDUP(ROUNDUP((uint64_t)addr, PGSIZE) + length, PGSIZE);
        if (upper_addr > (void *)USERTOP) {
            warn("addr 0x%p over USERTOP", addr);
            return EINVAL;
        }
        // 1.1.2 MAP_FIXEDが指定されている場合
        if (flags & MAP_FIXED) {
            // 1.1.2.1 アドレスはページ境界にあること
            if (NOT_PAGEALIGN(addr)) {
                warn("fixed address should be page align: 0x%p", addr);
                return -EINVAL;
            }
            if (prot == PROT_NONE) {
                int mapped = 0;
                for (int i = 0; i < length / PGSIZE; i++) {
                    uint64_t *pte = pgdir_walk(p->pgdir, (void *)((uint64_t)addr + i * PGSIZE), 0);
                    // すでにマッピング済み。
                    if (*pte & PTE_VALID) {
                        mapped++;
                        *pte &= ~PTE_USER;
                    }
                }
                if (mapped == (length / PGSIZE)) {
                    return addr;
                } else {
                    warn("PROT_NONE invalid addr");
                    return -EINVAL;
                }
            } else {
                // 1.1.2.2 指定されたアドレスが使用されていないこと
                struct mmap_region *tmp = find_mmap_region(addr);
                if (tmp && addr == tmp->addr) {
                    warn("addr is used");
                    return -EINVAL;
                }
                // 1.1.2.3 指定されたアドレスが使用可能なこと
                if (!is_usable(addr, length)) {
                    warn("addr is not available");
                    return -EINVAL;
                }
            }
        // 1.1.3 MAP_FIXEDが指定されていない場合は、アドレスを丸め下げ
        } else {
            if (NOT_PAGEALIGN(addr))
                addr = ROUNDDOWN(addr, PGSIZE);
            goto select_addr;
        }
    // 1.2. アドレスが指定されていない場合
    } else {
        // 1.2.1 最初のアドレス候補
        if (p->regions)
            addr = p->regions->addr;
        else
            addr = (void *)MMAPBASE;
select_addr:
        struct mmap_region *node = p->regions;
        while (node) {
            trace("- addr=0x%p, node->addr=0x%p, node->next->addr=0x%p", addr, node->addr, node->next ? node->next->addr : NULL);
            // 1.2.31 作成マッピングが現在のノードアドレスより小さい場合はこの候補を使用する
            if (addr + ROUNDUP(length, PGSIZE) <= node->addr)
                break;
            // 1.2.2 次のマッピングがない、または次のマッピングとの間における場合はこの候補を使用する
            if (node->addr + node->length <= addr && (node->next == 0 || addr + ROUNDUP(length, PGSIZE) <= node->next->addr))
                break;
            // 1.2.5 それ以外は、現在のマッピングの右端をアドレス候補とする
            if (addr <= node->addr + node->length)
                addr = node->addr + node->length;
            // 1.2.6 次のマッピングと比較する
            node = node->next;
        }
    }
    // 1.3 決定したアドレスがマップ範囲に含まれていることをチェックする
    if (addr + ROUNDUP(length, PGSIZE) > (void *)USERTOP)
        return -ENOMEM;

    // 1.4 MAX_FIXEDですでにマッピングがある場合、エラーとする
/* FIXME: 必要か検討
    // MAX_FIXEDですでにマッピングがある場合は書き換える
    struct mmap_region *tmp = find_mmap_region(addr);
    if (tmp) {
        if (addr == tmp->addr) {
            if (addr + length >= tmp->addr + tmp->length) {
                error = munmap(tmp->addr, tmp->length);
            } else {
                error = munmap(tmp->addr, length);
            }
        } else {
            error = munmap(addr, length);
        }
        if (error != 0) return error;
        return mmap(addr, length, prot, flags, f, offset);
    }
*/
    // 2. 新規mmap_regionを作成する

    // 2.1 mmap_regionのためのメモリを割り当てる
    struct mmap_region *region = (struct mmap_region*)kmalloc(sizeof(struct mmap_region));
    if (region == NULL)
        return -ENOMEM;
    // 2.2 mmap_regionにデータを設定する
    region->addr   = addr;
    region->length = length;
    region->flags  = flags;
    region->offset = offset;
    region->prot   = prot;
    region->next   = 0;

    // 2.2 fを設定
    if (f) {
        filedup(f);
        if (!(flags & MAP_ANONYMOUS)) {
            region->f = filedup(f);
        } else {
            goto out;
        }
    } else {
        region->f = NULL;
    }
    // 2.3 オリジナルフラグを設定: FIXME このフラグの必要性は?
    region->original = 1;

    // 3. p->regionsに作成したmmap_regionを追加する

    // 3.1 これがプロセスの最初のmmap_regionの場合はp->regionsに追加する
    if (p->nregions == 0) {
        p->regions = region;
        goto load_pages;
    }

    // 3.2 そうでない場合は適切な位置に追加して、p->regionsを更新する
    struct mmap_region *node = p->regions;
    struct mmap_region *prev = p->regions;
    while (node) {
        //cprintf("addr=0x%p, node->addr=0x%p\n", addr, node->addr);
        if (addr < node->addr) {
            region->next = node;
            prev = region;
            break;
        } else if (addr > node->addr && node->next && addr < node->next->addr) {
            region->next = node->next;
            node->next = region;
            break;
        }
        if (!node->next) {
            node->next = region;
            break;
        }
        node = node->next;
    }
    p->regions = prev;

load_pages:
    // 4. ページにマッピングする
    if ((error = mmap_load_pages(addr, length, prot, flags, f, offset)) < 0)
        goto out;
    //uvm_switch(p->pgdir);

    p->nregions++;
    region->addr = addr;
    // ファイルオフセットを正しく処理するためにlengthはここで切り上げる
    region->length = ROUNDUP(length, PGSIZE);
    //print_mmap_list(p, "mmap");

    return (long)region->addr;

out:
    if (f) fileclose(f);
    kmfree(region);
    return error;
}

// sys_munmapのメイン関数
long
munmap(void *addr, size_t length)
{
    struct proc *p = thisproc();
    long error;

    // addrはページ境界になければならない
    if ((uint64_t)addr & (PGSIZE - 1))
        return -EINVAL;
    // lengthは境界になくてもよいが、処理は境界に合わせる
    length = ROUNDUP(length, PGSIZE);

    debug("addr=0x%p, length=0x%llx", addr, length);

    struct mmap_region *region = find_mmap_region(addr);
    if (region == NULL) return 0;

    debug(" - found: addr=0x%p", region->addr);
    // ファイルが背後にある共有マップは書き戻し
    if ((region->flags & MAP_SHARED) && region->f && (region->prot & PROT_WRITE)) {
        begin_op();
        error = writei(region->f->ip, region->addr, region->offset, region->length);
        end_op();
        if (error < 0)
            return error;
    }

    // unmap
    if (region->length <= length) {
        if (region->f) {
            fileclose(region->f);
        }
        delete_mmap_node(p, region);
        p->nregions -= 1;
    } else {
        int free = (region->flags & MAP_SHARED) ? 0 : 1;
        if (length / PGSIZE) {
            uvm_unmap(p->pgdir, (uint64_t)addr, length / PGSIZE, free);
        }
        region->addr += length;
        region->length -= length;
        debug("new region: addr=0x%p, length=0x%llx", region->addr, region->length);
    }
    return 0;
}


void *
mremap(void *old_addr, size_t old_length, size_t new_length, int flags, void *new_addr)
{
    void *mapped_addr;
    long error = -EINVAL;
    //cprintf("- remap: old_addr=0x%p, old_length=0x%llx, new_length=0x%llx, flags=0x%x\n",
    //    old_addr, old_length, new_addr, flags);
    struct mmap_region *region = find_mmap_region(old_addr);
    if (region == NULL) return (void *)error;

    if (region->length != old_length) return  (void *)error;

    // 1: その場で拡大（縮小）可能の場合
    if (!region->next || region->addr + new_length <= region->next->addr) {
        if (flags & MREMAP_FIXED) {
            if ((error = munmap(old_addr, old_length)) < 0)
                return  (void *)error;
            return mmap(new_addr, new_length, region->prot, (region->flags | MAP_FIXED), region->f,  region->offset);
        } else {
            if ((error = scale_mmap_region(region, new_length)) < 0)
                return (void *)error;
            //uvm_switch(thisproc()->pgdir);
            return region->addr;
        }
    }
    // 2: その場では拡大できない場合
    if (!(flags & MREMAP_MAYMOVE)) return (void *)error;

    if (MREMAP_FIXED)
        mapped_addr = (void *)mmap(new_addr, new_length, region->prot, (region->flags | MAP_FIXED), region->f, region->offset);
    else
        mapped_addr = (void *)mmap(new_addr, new_length, region->prot, (region->flags & ~MAP_FIXED), region->f, region->offset);

    if (IS_ERR(mapped_addr)) {
        return mapped_addr;

    }

    memmove(new_addr, region->addr, region->length);
    if ((error = msync(mapped_addr, new_length, MS_SYNC)) < 0)
        return (void *)error;

    if ((error = munmap(region->addr, region->length)) < 0)
        return (void *)error;
    //uvm_switch(thisproc()->pgdir);
    //print_mmap_list(thisproc(), "mremap");
    return mapped_addr;
}

long
msync(void *addr, size_t length, int flags)
{
    struct proc *p = thisproc();
    long error = -EINVAL;

    if (!(flags & MS_ASYNC) && !(flags & MS_SYNC)) {
        flags |= MS_ASYNC;
    }

    if (flags & MS_ASYNC) {
        // Since  Linux  2.6.19, MS_ASYNC  is  in  fact  a no-op (from man(2))
        return 0;
    }

    // addrはページ境界になければならない。
    if (NOT_PAGEALIGN((uint64_t)addr)) return error;

    struct mmap_region *region = p->regions;
    while (region) {
        if (region->addr == addr) {
            if ((region->flags & MAP_SHARED) && (region->prot & PROT_WRITE) && region->f) {
                size_t len = region->length < length ? region->length : length;
                begin_op();
                error = writei(region->f->ip, region->addr, region->offset, len);
                end_op();
                if (error < 0)
                    return error;
                addr += len;
                length -= len;
                if (length <= 0) break;
            }

        }
        region = region->next;
    }
    return 0;
}


// struct mmap_region listを出力
void
print_mmap_list(struct proc *p, const char *title)
{
    cprintf("== PRINT mmap_region[%d] (%s): regions=0x%p, nregions=%d ==\n", p->pid, title, p->regions, p->nregions);
    if (p->nregions == 0) return;

    struct mmap_region *region = p->regions;
    int i=0;
    while (region) {
        cprintf(" - region[%d]: addr=0x%p, length=0x%llx, prot=0x%x, flags=0x%x, f=%d, offset=0x%llx\n",
            ++i, region->addr, region->length, region->prot, region->flags, (region->f ? region->f->ip->inum : 0), region->offset);
        region = region->next;
    }
}


// 親プロセスから子プロセスにmmap_regionをコピー(parent->nregions > 0はチェック済み)
long
copy_mmap_list(struct proc *parent, struct proc *child)
{
    uint64_t *ptep, *ptec;
    long error;

    struct mmap_region *node = parent->regions;
    struct mmap_region *cnode = 0, *tail = 0;

    while (node) {
        struct mmap_region *region = (struct mmap_region *)kmalloc(sizeof(struct mmap_region));
        if (region == (struct mmap_region *)0)
            return -ENOMEM;
        copy_mmap_region(region, node);
    /*
        if (node->flags & MAP_SHARED) {
            ptep = pgdir_walk(parent->pgdir, node->addr, 0);
            if (!ptep) panic("parent pgdir not pte: va=0x%p\n", region->addr);
            ptec = pgdir_walk(child->pgdir, region->addr, 0);
            if (!ptec) panic("child  pgdir not pte: va=0x%p\n", region->addr);
            uint64_t pa = PTE_ADDR(*ptec);
            kfree(P2V(pa));
            *ptec = *ptep;
            debug("*ptec[0]=0x%llx", *(uint64_t *)P2V(PTE_ADDR(*ptec)));
            debug("ptep=0x%llx, *ptep=0x%llx", ptep, *ptep);
            debug("ptec=0x%llx, *ptec=0x%llx", ptec, *ptec);
        }
    */
        if (cnode == 0)
            cnode = region;
        else
            tail->next = region;

        tail = region;
        node = node->next;
    }

    child->regions = cnode;
    child->nregions = parent->nregions;
    debug("child nregions=%d, regions=0x%p\n", child->nregions, child->regions);
    //print_mmap_list(parent, parent->name);
    //print_mmap_list(child, "child");
    uvm_switch(child->pgdir);
    uvm_switch(parent->pgdir);
    return 0;
}

long
copy_mmap_list2(struct proc *parent, struct proc *child)
{
    void *start;
    uint64_t *pte;
    uint64_t pa, perm;
    long error;

    struct mmap_region *node = parent->regions;
    struct mmap_region *cnode = 0, *tail = 0;

    while (node) {
        struct mmap_region *region = (struct mmap_region *)kmalloc(sizeof(struct mmap_region));
        if (region == (struct mmap_region *)0)
            return -ENOMEM;
        copy_mmap_region(region, node);

        start = node->addr;
        for (; start < node->addr + node->length; start += PGSIZE) {
            pte = pgdir_walk(parent->pgdir, start, 0);
            if (!pte) panic("parent pgdir not pte: va=0x%p\n", start);
            pa = PTE_ADDR(*pte);
            perm = PTE_FLAGS(*pte);
            // MAP_PRIVATEの場合は、READ ONLYで割り当て
            if (node->flags & MAP_PRIVATE)
                perm |= PTE_RO;
            if ((error = uvm_map(child->pgdir, start, PGSIZE, pa)) < 0)
                goto bad;
        }
        if (cnode == 0)
            cnode = region;
        else
            tail->next = region;

        tail = region;
        node = node->next;
    }

    child->regions = cnode;
    child->nregions = parent->nregions;
    debug("child nregions=%d, regions=0x%p\n", child->nregions, child->regions);
    //print_mmap_list(parent, parent->name);
    //print_mmap_list(child, "child");
    uvm_switch(child->pgdir);
    uvm_switch(parent->pgdir);
    return 0;
bad:
    free_mmap_list(child);
    return error;
}

// Copy on Write機能を実装（trap.cでFLT_PERMISSIONの場合に呼び出される）
long
copy_mmap_pages(void *addr, size_t length, uint64_t perm)
{
    uint64_t *pte;
    debug("copy_mmap_pages: addr=%p, length=0x%llx, perm=0x%llx\n", addr, length, perm);
    void *start  = addr;
    for (; start < addr + length; start += PGSIZE) {
        pte = pgdir_walk(thisproc()->pgdir, start, 0);
        if (pte == 0) { warn("copy_mmap_pages: pte = 0\n"); return -EINVAL; }
        uint64_t pa = PTE_ADDR(*pte);
        char *page = kalloc();
        if (!page) { warn("copy_mmap_pages: no page available\n"); return -ENOMEM; }
        memmove(page, P2V(pa), PGSIZE);
        *pte = V2P(page) | perm;
        debug("- start=%p, pte=%p, *pte=0x%llx\n", start, pte, *pte);
    }
    return 0;
}



/* nextとしてstartから始まるregionを追加可能なregionを探す */
struct mmap_region *
find_available_region(void *start)
{
    struct mmap_region *region = thisproc()->regions;

    while (region) {
        if (region->addr + region->length < start && (start < region->next->addr || region->next == 0))
            return region;
        region = region->next;
    }

    return (struct mmap_region *)-1;
}

/* startを含むregionを探す */
struct mmap_region *
find_mmap_region(void *start)
{
    struct mmap_region *region = thisproc()->regions;

    while (region) {
        if (region->addr <= start && start < (region->addr + region->length))
            return region;
        region = region->next;
    }
    return NULL;
}
