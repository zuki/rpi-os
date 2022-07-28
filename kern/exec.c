#include "types.h"
#include "exec.h"
#include "linux/auxvec.h"
#include "linux/elf.h"
#include "linux/elf-em.h"
#include "linux/errno.h"
#include "linux/mman.h"
#include "linux/capability.h"
#include "trap.h"
#include "file.h"
#include "log.h"
#include "string.h"
#include "console.h"
#include "vm.h"
#include "proc.h"
#include "mm.h"
#include "memlayout.h"
#include "mmap.h"
#include "kmalloc.h"
#include "pagecache.h"
#include "syscall1.h"

// elf_bssからページ境界までゼロクリアする
// |----elf_bss_0000000000000000|
static void padzero(uint64_t elf_bss)
{
    uint64_t nbyte;

    nbyte = ELF_PAGEOFFSET(elf_bss);
    debug("elf_bss=0x%llx, nbyte=0x%llx", elf_bss, nbyte ? (ELF_MIN_ALIGN - nbyte) : 0);
    if (nbyte) {
        nbyte = ELF_MIN_ALIGN - nbyte;
        memset((void *)elf_bss, 0, nbyte);
    }
}

// bss領域用のマッピングを必要であれば作成する
static void *set_brk(uint64_t start, uint64_t end)
{
    void  *old_start, *old_end;

    old_start = (void *)start;
    old_end = (void *)end;
    start = ELF_PAGEALIGN(start);   // roundup
    end = ELF_PAGEALIGN(end);       // roundup
    // 既存のマッピング（データ用）で間に合う場合は何もしない（既存のマッピングは0詰め済み）
    if (end <= start) {
        padzero((uint64_t)old_start);
        return old_start;
    }

    debug("old_start=0x%p, start=0x%llx, old_end=0x%p, end=0x%llx",
        old_start, start, old_end, end);
    return (void *)mmap((void *)start, end - start, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, NULL, 0);
}

struct file *
get_file(char *path)
{
    struct inode *ip;
    struct file *f;
    char buf[512];
    int n;
    long error;

    begin_op();
loop:
    if ((ip = namei(path)) == 0) {
        end_op();
        return (void *)-ENOENT;
    }
    ilock(ip);

    if (ip->type == T_SYMLINK) {
        if ((n = readi(ip, buf, 0, sizeof(buf) - 1)) <= 0) {
            warn("couldn't read sysmlink target");
            error = -ENOENT;
            goto bad;
        }
        buf[n] = 0;
        path = buf;
        iunlockput(ip);
        goto loop;
    }

    if (ip->type != T_FILE) {
        error = -EINVAL;
        goto bad;
    }

    if ((f = filealloc()) == 0) {
        error = -ENOSPC;
        goto bad;
    }

    if ((error = (long)permission(ip, MAY_READ)) < 0) {
        goto bad;
    }

    iunlockput(ip);
    end_op();

    f->type     = FD_INODE;
    f->ref      = 1;
    f->ip       = ip;
    f->off      = 0;
    f->flags    = O_RDONLY;
    f->readable = 1;
    f->writable = 0;
    return f;

bad:
    iunlockput(ip);
    end_op();
    return (void *)error;
}

// 親から受け継いだ不要な情報を破棄する
static void flush_old_exec(void)
{
    struct proc *p = thisproc();

    // (1) signalのflush
    flush_signal_handlers(p);
    // (2) close_on_execのfileのclose
    for (int i = 0; i < NOFILE; i++) {
        if (p->ofile[i] && bit_test(p->fdflag, i)) {
            fileclose(p->ofile[i]);
            p->ofile[i] = 0;
            bit_remove(p->fdflag, i);
        }
    }
    // (3) capability 再設定
    cap_clear(p->cap_inheritable);
    cap_clear(p->cap_permitted);
    cap_clear(p->cap_effective);

    if (p->uid == 0 || p->euid == 0) {
        cap_set_full(p->cap_inheritable);
        cap_set_full(p->cap_permitted);
    }

    if (p->euid == 0 || p->fsuid == 0)
        cap_set_full(p->cap_effective);
}

static uint64_t
load_interpreter(char *path, uint64_t *base)
{
    struct file *f;
    Elf64_Ehdr elf;
    Elf64_Phdr *phdr;                       // プログラムヘッダ作業用
    Elf64_Phdr *phdata;                     // 全プログラムヘッダ読み込み用
    uint64_t load_addr = 0;                 // インタプリタロードアドレス
    uint64_t last_bss = 0, elf_bss = 0;     // 最新と現在のbssアドレス
    void *mapped;                           // elf_mmapしたアドレス
    size_t size;
    int i, set = 0;
    long error = -ENOEXEC;

    f = get_file(path);
    if (IS_ERR(f))
        return (uint64_t)f;
    //if (readi(f->ip, (char *)&elf, 0, sizeof(elf)) != sizeof(elf)) {
    if ((error = copy_page(f->ip, 0, (char *)&elf, sizeof(elf), 0)) < 0) {
        warn("readelf bad");
        goto out;
    }

    /* 簡単な正当性チェック */
    error = -ENOEXEC;
    if (elf.e_type != ET_DYN && elf.e_type != ET_EXEC)
        goto out;
    if (!ELF_CHECK_ARCH(&elf))
        goto out;
    if (elf.e_phentsize != sizeof(Elf64_Phdr))
        goto out;
    if (elf.e_phnum > ELF_MIN_ALIGN / sizeof(Elf64_Phdr))
        goto out;

    size = elf.e_phentsize * elf.e_phnum;
    if (size > ELF_MIN_ALIGN)
        goto out;
    phdata = (Elf64_Phdr *)kmalloc(size);
    if (!phdata)
        goto out;

    //if (readi(f->ip, (char *)phdata, elf.e_phoff, size) != size)
    if ((error = copy_page(f->ip, 0, (char *)phdata, size, elf.e_phoff)) < 0)
        goto free_phdata;

    phdr = phdata;
    for (i = 0; i < elf.e_phnum; i++, phdr++) {
        if (phdr->p_type != PT_LOAD)
            continue;
        int flags = 0;
        int prot  = 0;
        uint64_t vaddr = 0;
        uint64_t k;

        if (phdr->p_flags & PF_R) prot |= PROT_READ;
        if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
        if (phdr->p_flags & PF_X) prot |= PROT_EXEC;

        if (prot & PROT_WRITE)
            flags = MAP_PRIVATE;
        else
            flags = MAP_SHARED | MAP_DENYWRITE;

        vaddr = phdr->p_vaddr;
        if (elf.e_type == ET_EXEC || set)
            flags |= MAP_FIXED;
        else
            load_addr = ELF_PAGESTART(ELF_ET_DYN_BASE - vaddr);

        mapped = (void *)mmap(
            (void *)ELF_PAGESTART(load_addr + vaddr),
            phdr->p_filesz + ELF_PAGEOFFSET(vaddr),
            prot, flags, f,
            phdr->p_offset - ELF_PAGEOFFSET(vaddr));
        if (IS_ERR(mapped)) {
            error = (long)mapped;
            goto free_phdata;
        }
        if (set == 0 && elf.e_type == ET_DYN) {
            load_addr = (uint64_t)mapped - ELF_PAGESTART(vaddr);
            set = 1;
        }

        k = load_addr + vaddr + phdr->p_filesz;
        if (k > elf_bss) elf_bss = k;

        k = load_addr + vaddr + phdr->p_memsz;
        if (k > last_bss) last_bss = k;
    }

    padzero(elf_bss);
    elf_bss = ELF_PAGESTART(elf_bss + ELF_MIN_ALIGN - 1);

    if (last_bss > elf_bss) {
        mapped = set_brk(elf_bss, last_bss);
        if (IS_ERR(mapped)) {
            error = (long)mapped;
            goto free_phdata;
        }
    }
    *base = load_addr;
    error = ((uint64_t)elf.e_entry) + load_addr;

free_phdata:
    if (phdata)
        kmfree((char *)phdata);
out:
    fileclose(f);
    return (uint64_t)error;
}

int
execve(const char *path, char *const argv[], char *const envp[])
{
    trace("[%d] parse %s", thisproc()->pid, path);
    char *s, *interp;
    int has_interp = 0;
    uint64_t interp_entry, interp_base;
    long error = -EACCES;
    Elf64_Ehdr elf;                         // ELFヘッダ
    Elf64_Phdr *phdr;                       // プログラムヘッダ作業用
    Elf64_Phdr *phdata;                     // 全プログラムヘッダ読み込み用

    if (thisproc()->pid == 11) {
        debug("argv[0]: %s, envp[0]: %s, envp[1]: %s", argv[0], envp[0], envp[1]);
    }

    if (fetchstr((uint64_t) path, &s) < 0)
        return -EACCES;

    // Save previous page table.
    struct proc *curproc = thisproc();
    void *oldpgdir = curproc->pgdir, *pgdir = vm_init();
    struct inode *ip = 0;

    if (pgdir == 0) {
        warn("vm init failed");
        goto bad;
    }

    begin_op();
    ip = namei(path);
    if (ip == 0) {
        end_op();
        warn("namei bad");
        goto bad;
    }
    ilock(ip);

    trace("path='%s', proc: uid=%d, gid=%d, ip: inum=%d, mode=0x%x, uid=%d, gid=%d", s, curproc->uid, curproc->gid, ip->inum, ip->mode, ip->uid, ip->gid);

    //if (readi(ip, (char *)&elf, 0, sizeof(elf)) != sizeof(elf)) {
    if ((error = copy_page(ip, 0, (char *)&elf, sizeof(elf), 0)) < 0) {
        warn("readelf bad");
        goto bad;
    }

    error = -EACCES;
    if (!
        (elf.e_ident[EI_MAG0] == ELFMAG0 && elf.e_ident[EI_MAG1] == ELFMAG1
         && elf.e_ident[EI_MAG2] == ELFMAG2
         && elf.e_ident[EI_MAG3] == ELFMAG3)) {
        warn("elf header magic invalid");
        goto bad;
    }
    if (elf.e_ident[EI_CLASS] != ELFCLASS64) {
        warn("64 bit program not supported");
        goto bad;
    }
    // ELF 64-bit LSB pie executableのe_typeはET_DYN
    if (elf.e_type != ET_EXEC && elf.e_type != ET_DYN) {
        warn("bad header type %d", elf.e_type);
        goto bad;
    }
    trace("check elf header finish");

    size_t size = elf.e_phentsize * elf.e_phnum;
    if (size > ELF_MIN_ALIGN)
        goto bad;
    phdata = (Elf64_Phdr *)kmalloc(size);
    if (!phdata)
        goto bad;

    if ((error = copy_page(ip, 0, (char *)phdata, size, elf.e_phoff)) < 0)
        goto free_phdata;

    curproc->pgdir = pgdir;     // Required since readi(sdrw) involves context switch(switch page table).

    // Set-uid, Set-gidの処理
    if (ip->mode & S_ISUID && curproc->uid != 0)
        curproc->fsuid = ip->uid;
    else
        curproc->fsuid = curproc->uid;
    if (((ip->mode & (S_ISGID | S_IXGRP)) == (S_ISGID | S_IXGRP)) && curproc->gid != 0)
        curproc->fsgid = ip->gid;
    else
        curproc->fsgid = curproc->gid;

    flush_old_exec();

    // Load program into memory.
    size_t sz = 0, base = 0, stksz = 0;
    int first = 1;
    error = -EACCES;

    phdr = phdata;
    for (int i = 0; i < elf.e_phnum; i++, phdr++) {
        // interpreterの文字列取得
        if (phdr->p_type == PT_INTERP) {
            if (phdr->p_filesz > PGSIZE)       // インタプリタ名が長すぎる
                goto bad;
            interp = (char *)kmalloc(phdr->p_filesz);
            if (!interp)
                goto bad;
            if ((error = copy_page(ip, 0, interp, (uint32_t)phdr->p_filesz, (uint32_t)phdr->p_offset)) < 0)
                goto free_interp;
            has_interp = 1;
            continue;
        }
        if (phdr->p_type != PT_LOAD) {
            debug("unsupported type 0x%x, skipped\n", phdr->p_type);
            continue;
        }
        error = -EACCES;
        if (phdr->p_memsz < phdr->p_filesz) {
            warn("memsz smaller than filesz");
            goto bad;
        }

        if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr) {
            warn("vaddr + memsz overflow");
            goto bad;
        }

        if (first) {
            first = 0;
            sz = base = phdr->p_vaddr;
            if (base % PGSIZE != 0) {
                warn("first section should be page aligned!");
                goto bad;
            }
        }

        if ((sz = uvm_alloc(pgdir, base, stksz, sz, phdr->p_vaddr + phdr->p_memsz)) == 0) {
            warn("uvm_alloc bad");
            goto bad;
        }

        uvm_switch(pgdir);
        debug("vaddr: 0x%llx, filesz: 0x%llx, offset: 0x%llx", phdr->p_vaddr, phdr->p_filesz, phdr->p_offset);
        if ((error = (copy_pages(ip, (char *)phdr->p_vaddr, phdr->p_filesz, phdr->p_offset))) < 0) {
            warn("copy_pages failed");
            goto bad;
        }

        // Initialize BSS.
        memset((void *)phdr->p_vaddr + phdr->p_filesz, 0,
               phdr->p_memsz - phdr->p_filesz);

        // Flush dcache to memory so that icache can retrieve the correct one.
        dccivac((void *)phdr->p_vaddr, phdr->p_memsz);

        debug("init bss [0x%p, 0x%p)", phdr->p_vaddr + phdr->p_filesz,
              phdr->p_vaddr + phdr->p_memsz);
    }

    iunlockput(ip);
    end_op();
    ip = 0;

    if (has_interp) {
        debug("load interpreter");
        interp_entry = load_interpreter(interp, &interp_base);
        if (IS_ERR((void *)interp_entry))
            goto free_interp;
    }

    // Push argument strings, prepare rest of stack in ustack.
    uvm_switch(oldpgdir);
    char *sp = (char *)USERTOP;
    int argc = 0, envc = 0;
    size_t len;

    uint64_t platform = 0;
    if (has_interp) {
        len = strlen(ELF_PLATFORM) + 1;
        sp -= len;
        platform = (uint64_t)sp;
        if (copyout(pgdir, sp, ELF_PLATFORM, len) < 0) {
            goto free_interp;
        }
    }
    if (argv) {
        for (; in_user((void *)(argv + argc), sizeof(*argv)) && argv[argc];
             argc++) {
            if ((len = fetchstr((uint64_t) argv[argc], &s)) < 0) {
                warn("argv fetchstr bad");
                goto free_interp;
            }
            trace("argv[%d] = '%s', len: %d", argc, argv[argc], len);
            sp -= len + 1;
            if (copyout(pgdir, sp, argv[argc], len + 1) < 0)    // include '\0';
                goto free_interp;
        }
    }
    if (envp) {
        for (; in_user((void *)(envp + envc), sizeof(*envp)) && envp[envc];
             envc++) {
            if ((len = fetchstr((uint64_t) envp[envc], &s)) < 0) {
                warn("envp fetchstr bad");
                goto free_interp;
            }
            trace("envp[%d] = '%s', len: %d", envc, envp[envc], len);
            sp -= len + 1;
            if (copyout(pgdir, sp, envp[envc], len + 1) < 0)    // include '\0';
                goto free_interp;
        }
    }

    // auxv
    uint64_t auxv_size;
    uint64_t auxv_dyn[][2] = {
        { AT_PHDR,    elf.e_phoff },
        { AT_PHENT,   sizeof(Elf64_Phdr) },
        { AT_PHNUM,   elf.e_phnum },
        { AT_PAGESZ,  PGSIZE },
        { AT_BASE,    interp_base },
        { AT_FLAGS,   0 },
        { AT_ENTRY,   elf.e_entry },
        { AT_UID,     (uint64_t) curproc->uid },
        { AT_EUID,    (uint64_t) curproc->euid },
        { AT_GID,     (uint64_t) curproc->gid },
        { AT_EGID,    (uint64_t) curproc->egid },
        { AT_PLATFORM, platform },
        { AT_HWCAP,   ELF_HWCAP },
        { AT_CLKTCK,  HZ },
        { AT_NULL,    0 }
    };
    uint64_t auxv_sta[][2] = { { AT_PAGESZ, PGSIZE } };

    if (has_interp) {
        auxv_size = sizeof(auxv_dyn);
    } else {
        auxv_size = sizeof(auxv_sta);
    }

    // Align to 16byte. 3 zero terminator of auxv/envp/argv and 1 argc.
    void *newsp =
        (void *)ROUNDDOWN((size_t)sp - auxv_size -
                          (envc + argc + 4) * 8, 16);
    if (copyout(pgdir, newsp, 0, (size_t)sp - (size_t)newsp) < 0)
        goto free_interp;

    uvm_switch(pgdir);

    uint64_t *newargv = newsp + 8;
    uint64_t *newenvp = (void *)newargv + 8 * (argc + 1);
    uint64_t *newauxv = (void *)newenvp + 8 * (envc + 1);
    if (thisproc()->pid ==11)
        debug("argc: %d, envc: %d, auxc: %d, argv: 0x%p, envp: 0x%p, auxv: 0x%p", argc, envc, auxv_size/16, newargv, newenvp, newauxv);
    if (has_interp)
        memmove(newauxv, auxv_dyn, auxv_size);
    else
        memmove(newauxv, auxv_sta, auxv_size);

    for (int i = envc - 1; i >= 0; i--) {
        newenvp[i] = (uint64_t) sp;
        if (thisproc()->pid == 11)
            debug("newenvp[%d]: 0x%p = %s", i, newenvp + i, newenvp[i]);
        for (; *sp; sp++) ;
        sp++;
    }
    for (int i = argc - 1; i >= 0; i--) {
        newargv[i] = (uint64_t) sp;
        if (thisproc()->pid == 11)
            debug("newargv[%d]: 0x%p = %s", i, newargv + i, newargv[i]);
        for (; *sp; sp++) ;
        sp++;
    }
    *(size_t *)(newsp) = argc;

    sp = newsp;
    // Allocate user stack.
    stksz = ROUNDUP(USERTOP - (uint64_t)sp, 10 * PGSIZE);
    if (thisproc()->pid ==11) {
        debug("stksz: 0x%llx, sp: 0x%p", stksz, sp);
        debug("copyout: 0x%llx, 0, 0x%llx", (USERTOP - stksz), stksz - (USERTOP - (uint64_t)sp));
    }
    if (copyout
        (pgdir, (void *)(USERTOP - stksz), 0,
         stksz - (USERTOP - (uint64_t)sp)) < 0)
        goto free_interp;

/*
    if (thisproc()->pid ==11) {
        uint64_t num = USERTOP - (uint64_t)sp;
        uint64_t *pos;
        char c;
        if (num & 0x01) num += 8;
        num = num / 8;
        for (int j = 1; j <= num; j++) {
            pos = (uint64_t *)(USERTOP - j * 16);
            cprintf("%llx: %016llx %016llx", pos, *(pos), *(pos + 1));
            cprintf(" ");
            for (int k = 0; k < 16; k++) {
                c = *((char *)(pos) + k);
                if (c < 0x20 || c > 0x7e) c = '.';
                cprintf("%c", c);
            }
            cprintf("\n");
        }
    }
*/
    assert((uint64_t) sp > USERTOP - stksz);

    // Commit to the user image.
    curproc->pgdir = pgdir;

    curproc->base = base;
    curproc->sz = sz;
    curproc->stksz = stksz;

    // memset(curproc->tf, 0, sizeof(*curproc->tf));

    if (has_interp)
        curproc->tf->elr = interp_entry;
    else
        curproc->tf->elr = elf.e_entry;
    curproc->tf->sp = (uint64_t) sp;
    if (thisproc()->pid == 11) {
        debug("entry 0x%p", curproc->tf->elr);
    }

    uvm_switch(oldpgdir);

    // Save program name for debugging.
    const char *last, *cur;
    for (last = cur = path; *cur; cur++)
        if (*cur == '/')
            last = cur + 1;
    safestrcpy(curproc->name, last, sizeof(curproc->name));

    uvm_switch(curproc->pgdir);
    vm_free(oldpgdir);
    if (thisproc()->pid ==11) debug("run [%d] %s", curproc->pid, curproc->name);
    if (has_interp && interp)
        kmfree((void *)interp);
    //if (thisproc()->pid ==11) vm_stat(thisproc()->pgdir);
    return 0;

free_interp:
    if (has_interp && interp)
        kmfree((void *)interp);
free_phdata:
    if (phdata)
        kmfree((void *)phdata);
bad:
    if (pgdir)
        vm_free(pgdir);
    if (ip) {
        iunlockput(ip);
        end_op();
    }
    thisproc()->pgdir = oldpgdir;
    warn("bad");
    return error;
}

/* 1. 引数文字列をプッシュ
     *
     * 2. 初期スタックは次の通り
     *
     *   +-------------+
     *   | argv[argc-1]|  argv[argc-1]文字列
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   argv[0]   |  argv[0]文字列
     *   +-------------+
     *   | envp[envc-1]|  envp[envc-1]文字列
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   envp[0]   |  envp[0]文字列
     *   +-------------+
     *   | auxv[o] = 0 |  auxv[]の終わりのサイン
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   auxv[0]   |
     *   +-------------+
     *   | estack[m]=0 |  m == envc: estack[]の終わりのサイン
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |  estack[0]  |  envp[0]文字列が存在するアドレス
     *   +-------------+
     *   | ustack[n]=0 |  n == argc: ustack[]の終わりのサイン
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   ustack[0] |  argv[0]文字列が存在するアドレス: tf->x1
     *   +-------------+
     *   |    argc     |  tf->x0 = argc
     *   +-------------+  <== sp
     *
     * ここで、ustack[i], estack[j] は8バイトのポインタ、auxv[k] は
     * 補助ベクタと呼ばれるものでカーネルレベルの情報をユーザ
     * プロセスに伝達するのに使用される。
     *
     * ## 例
     *
     * ```
     * sp -= 8; *(size_t *)sp = AT_NULL;    // auxv[3] auxv[]の終わりのサイン
     * sp -= 8; *(size_t *)sp = PGSIZE;     // auxv[2]
     * sp -= 8; *(size_t *)sp = AT_PAGESZ;  // auxv[1]
     *
     * sp -= 8; *(size_t *)sp = 0;          // estack[m]: estack[]の終わりのサイン
     *
     * // ここにestackを置く。envpを実装しない場合は無視する。
     *
     * sp -= 8; *(size_t *)sp = 0;          // ustack[n]: ustack[]の終わりのサイン
     *
     * // ここにustackを置く                // ustack[0] - ustack[n-1], ustack[n]=0
     *
     * sp -= 8; *(size_t *)sp = argc;       // argc
     *
     * // スタックポインタは必ず16バイトアライン。
     *
     * thisproc()->tf->sp = sp;
     * ```
     *
     */
