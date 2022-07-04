#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define MMAPBASE 0x600000000000UL

char *filename = "README";

// File backend mappings test (21 tests)
void file_private_test();
void file_shared_test();
void file_invalid_fd_test();
void file_invalid_flags_test();
void file_writeable_shared_mapping_on_ro_file_test();
void file_ro_shared_mapping_on_ro_file_test();
void file_exceed_size_test();
void file_exceed_count_test();
void file_empty_file_size_test();
void file_private_mapping_perm_test();
void file_pagecache_coherency_test();
void file_private_with_fork_test();
void file_shared_with_fork_test();
void file_mapping_with_offset_test();
void file_given_addr_test();
void file_invalid_addr_test();
void file_overlap_given_addr_test();
void file_intermediate_given_addr_test();
void file_intermediate_given_addr_not_possible_test();
void file_exceeds_file_size_test();
void file_mapping_on_wo_file_test();

// Anonymous tests (15 tests)
void anon_private_test();
void anon_shared_test();
void anon_private_fork_test();
void anon_shared_multi_fork_test();
void anon_exceed_size_test();
void anon_exceed_count_test();
void anon_zero_size_test();
void anon_missing_flags_test();
void anon_private_shared_fork_test();
void anon_given_addr_test();
void anon_invalid_addr_test();
void anon_overlap_given_addr_test();
void anon_intermediate_given_addr_test();
void anon_intermediate_given_addr_not_possible_test();
void anon_write_on_ro_mapping_test();

// Other Mmap tests (5 Tests)
void munmap_partial_size_test();
void mmap_write_on_ro_mapping_test();
void mmap_none_permission_test();
void mmap_valid_map_fixed_test();
void mmap_invalid_map_fixed_test();

void write_readme(int n);

void file_tests() {
    //write_readme(8192);
/*
    file_invalid_fd_test();
    file_invalid_flags_test();
    file_writeable_shared_mapping_on_ro_file_test();
    file_ro_shared_mapping_on_ro_file_test();
    file_private_mapping_perm_test();
    file_exceed_size_test();
    file_exceed_count_test();
    file_empty_file_size_test();  // FIXME: n > 32
*/
    file_private_test();
    file_shared_test();
    file_pagecache_coherency_test();
    file_private_with_fork_test();
    file_shared_with_fork_test();
    file_mapping_with_offset_test();
    file_given_addr_test();
    file_invalid_addr_test();
    file_overlap_given_addr_test();
    file_intermediate_given_addr_test();
    file_intermediate_given_addr_not_possible_test();
    file_exceeds_file_size_test();
    file_mapping_on_wo_file_test();
}

void anonymous_tests() {
    anon_private_test();
    anon_shared_test();
    anon_private_fork_test();
    anon_shared_multi_fork_test();
    anon_private_shared_fork_test();
    anon_missing_flags_test();
    anon_exceed_count_test();
    anon_exceed_size_test();
    anon_zero_size_test();
    anon_given_addr_test();
    anon_invalid_addr_test();
    anon_overlap_given_addr_test();
    anon_intermediate_given_addr_test();
    anon_intermediate_given_addr_not_possible_test();
}

void other_tests() {
    munmap_partial_size_test();
    mmap_write_on_ro_mapping_test();
    mmap_none_permission_test();
    mmap_valid_map_fixed_test();
    mmap_invalid_map_fixed_test();
}

char temp[8192], buf[8192];

void write_readme(int n) {
    for (int i = 0; i < n; i++) temp[i] = 'a';
    int fd = open(filename, O_CREAT | O_WRONLY, 0666);
    if (fd == -1) {
        printf("write_readme failed: at open\n");
        exit(1);
    }
    if (write(fd, temp, n) != n) {
        printf("write_readme failed: at write\n");
        exit(1);
    }
    close(fd);
}

// Utility strcmp function
int my_strcmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return i + 1;
        }
    }
    return 0;
}

int file_ok = 0, file_ng = 0, anon_ok = 0, anon_ng = 0, other_ok = 0,
    other_ng = 0;

int main(int args, char *argv[]) {
    file_tests();
    //anonymous_tests();
    //other_tests();
    printf("\nfile_test:  ok: %d, ng: %d\n", file_ok, file_ng);
    printf("anon_test:  ok: %d, ng: %d\n", anon_ok, anon_ng);
    printf("other_test: ok: %d, ng: %d\n", other_ok, other_ng);

    remove("README");

    return 0;
}

// <!!-------- ファイルが背後にあるマッピング ---------- !!>

// [F09] ファイルが背後にあるプライベートマッピングのテスト
// test 1: ファイルから読み込んだバッファ1とmmapしたアドレスから読み込んだデータが一致する
// test 2: バッファ1とアドレスデータの一部を同じように書き換えると双方は一致する
// test 3: 再度ファイルから読み込んだバッファ2とバッファ1は異なる（プライベートマッピングは
//         書き戻さないため）
void file_private_test() {
    printf("\n[F-09] ファイルが背後にあるプライベートマッピング\n");
    write_readme(1000);     // 'a'を1000バイト書き込み
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("file backed private mapping test failed: at open\n");
        goto f7_bad;
    }

    int size = 1000;
    volatile char buf[1000];
    int n = read(fd, buf, size);    // buf = 'a' x 1000
    if (n != size) {
        printf(
            "file backed private mapping test failed: at read (%d != 1000)\n",
            n);
        goto f7_bad;
    }
    volatile char *ret = (volatile char *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED) {
        printf("file backed private mapping test failed 1\n");
        goto f7_bad;
    }

    if (my_strcmp(buf, (char *)ret, size) != 0) {   // buf = ret = "a...a"
        printf("file backed private mapping test failed 2\n");
        goto f7_bad_unmap;
    }

    for (int i = 0; i < 64; i++) {
        ret[i] = 'p';
        buf[i] = 'p';
    }
/*
    printf("write(u) (%p): ", ret);
    int pos;
    for (pos = 0; pos < 70; pos++) {
        printf("%c", ret[pos]);
        if (((pos + 1) % 10) == 0) printf(" ");
    }
    printf("\n");
    printf("write(u) (%p): ", buf);
    for (pos = 0; pos < 70; pos++) {
        printf("%c", buf[pos]);
        if (((pos + 1) % 10) == 0) printf(" ");
    }
    printf("\n");
*/
    int pos;
    if ((pos = my_strcmp(buf, ret, size)) != 0) {   // buf = ret = 'p' x 40 + 'a' x 960
        printf("file backed private mapping test failed 3: pos: %d, buf: %c, ret: %c\n", pos - 1, buf[pos - 1], ret[pos - 1]);
        goto f7_bad_unmap;
    }
    int res = munmap((void *)ret, size);
    if (res == -1) {
        printf("file backed private mapping test failed 4\n");
        goto f7_bad;
    }
    int fd2 = open(filename, O_RDONLY);
    char buf2[1000];
    // Read from the file again and check if it is not equal to mapping data as
    // mapping is private
    n = read(fd2, buf2, size);
    if (n != size) {
        printf("file backed private mapping test failed: at read 2\n");
        goto f7_bad;
    }
    if (my_strcmp(buf, buf2, size) == 0) {
        printf("file backed private mapping test failed 5: pos: %d, buf: %c, buf2: %c\n", pos - 1, buf[pos - 1], buf2[pos - 1]);
        goto f7_bad;
    }
    close(fd2);
    close(fd);
    printf("[F-09] ok\n");
    file_ok++;
    return;

f7_bad_unmap:
    munmap((void *)ret, size);
f7_bad:
    file_ng++;
    printf("[F-09] failed\n");
    return;
}

// [F-10] ファイルが背後にある共有マッピングのテスト
// test 1: ファイルから読み込んだバッファ1とmmapしたアドレスから読み込んだデータが一致する
// test 2: バッファ1とアドレスデータの一部を同じように書き換えると双方は一致する
// test 3: 再度ファイルから読み込んだバッファ2とバッファ1も一致する（共有マッピングは
//         書き戻すため）
void file_shared_test() {
    printf("\n[F-10] ファイルが背後にある共有マッピングのテスト\n");
    write_readme(1000);
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("file backed shared mapping test failed: at open\n");
        file_ng++;
        printf("[F-10] failed\n");
        return;
    }
    int size = 1000;
    char buf[1000];
    int n = read(fd, buf, size);
    if (n != size) {
        printf("file backed shared mapping test failed: at read (%d != 1000)\n",
               n);
        file_ng++;
        printf("[F-10] failed\n");
        return;
    }
    char *ret = (char *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (ret == MAP_FAILED) {
        printf("file backed shared mapping test failed 1\n");
        file_ng++;
        printf("[F-10] failed\n");
        return;
    }
    // Check if both entries have same data
    if (my_strcmp(buf, ret, size) != 0) {
        printf("file backed shared mapping test failed 2\n");
        file_ng++;
        printf("[F-10] failed\n");
        return;
    }
    for (int i = 0; i < 40; i++) {
        ret[i] = 'a';
        buf[i] = 'a';
    }
    // Check if both mappings are same after edit
    if (my_strcmp(buf, ret, size) != 0) {
        printf("file backed shared mapping test failed 3\n");
        file_ng++;
        printf("[F-10] failed\n");
        return;
    }
    int res = munmap((void *)ret, size);
    if (res == -1) {
        printf("file backed shared mapping test failed 4\n");
        file_ng++;
        printf("[F-10] failed\n");
        return;
    }
    int fd2 = open(filename, O_RDONLY);
    char buf2[1000];
    // Read from the file again and check if it is equal to mapping data
    n = read(fd2, buf2, size);
    if (n != size) {
        printf("file backed shared mapping test failed: at read 2\n");
        file_ng++;
        printf("[F-10] failed\n");
        return;
    }
    if (my_strcmp(buf, buf2, size) != 0) {
        printf("file backed shared mapping test failed 5\n");
        file_ng++;
        printf("[F-10] failed\n");
        return;
    }
    close(fd2);
    close(fd);
    printf("[F-10] ok\n");
    file_ok++;
}

// <!!-------- File Backed mapping ---------- !!>

// [F-01] 不正なfdを指定した場合のテスト
// test 1: 負のfd, オープンされていないfd, 範囲外のfdを指定した場合はエラーになる
void file_invalid_fd_test() {
    printf("[F-01] 不正なfdを指定した場合のテスト\n");
    int size = 100;
    int fds[3] = {
        -1, 10,
        128};  // Negative fd, fd in range but does not exist, fd out of range
    for (int i = 0; i < 3; i++) {
        void *ret = (void *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE, fds[i], 0);
        if (ret == MAP_FAILED) {
            perror(" error");
            continue;
        }
        printf("[F-01] %d failed\n", i);
        file_ng++;
        return;
    }
    file_ok++;
    printf("[F-01] ok\n");
}

// [F-02] 不正なフラグを指定した場合のテスト
// test 1: 不正なフラグを指定した場合はエラーになる
void file_invalid_flags_test() {
    printf("\n[F-02] 不正なフラグを指定した場合のテスト\n");
    int fd = open(filename, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        printf("error: at open\n");
        file_ng++;
        return;
    }
    int size = 100;
    char *ret = (char *)mmap((void *)0, size, PROT_READ | PROT_WRITE, 0, fd, 0);
    if (ret == MAP_FAILED) {
        perror(" error");
        printf("[F-02] ok\n");
        close(fd);
        file_ok++;
        return;
    }
    file_ng++;
    printf("[F-02] failed\n");
    munmap((void *)ret, size);
}

// [F-03] readonlyファイルにPROT_WRITEな共有マッピングをした場合
// test 1: この指定をして場合はエラーになる
void file_writeable_shared_mapping_on_ro_file_test() {
    printf(
        "\n[F-03] readonlyファイルにPROT_WRITEな共有マッピングをした場合のテスト\n");
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        printf(
            "error: at open\n");
        file_ng++;
        return;
    }
    char *ret = mmap((void *)0, 200, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ret == MAP_FAILED) {
        perror(" error");
        printf("[F-03] ok\n");
        close(fd);
        file_ok++;
        return;
    }
    printf("[F-03] failed\n");
    file_ng++;
    munmap((void *)ret, 200);
}

// [F-04] readonlyファイルにPROT_READのみの共有マッピングをした場合のテスト
// test 1: マッピングは成功する
void file_ro_shared_mapping_on_ro_file_test() {
    printf(
        "\n[F-04] readonlyファイルにPROT_READのみの共有マッピングをした場合のテスト\n");
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        printf("error: at open\n");
        file_ng++;
        return;
    }
    void *ret = mmap((void *)0, 200, PROT_READ, MAP_SHARED, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-04] mmap failed\n");
        file_ng++;
        return;
    }
    //printf("- mapped addr=%p\n", ret);
    int res = munmap(ret, 200);
    if (res == -1) {
        printf("[F-04] munmap failed\n");
        file_ng++;
        return;
    }
    printf("[F-04] ok\n");
    close(fd);
    file_ok++;
}

// [F-05] read onlyなファイルに対するPROT指定の異なるプライベートマッピンのテスト
// test 1: PROT_READのみ指定した場合、成功する
// test 2: PROT_WRITEも指定した場合も成功し、書き込みも可能
void file_private_mapping_perm_test() {
    printf("\n[F-05] PROT指定の異なるプライベートマッピンのテスト\n");
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        printf("file backed private mapping permission test failed\n");
        file_ng++;
        return;
    }
    // read only private mapping on read only opened file
    char *ret = mmap((void *)0, 200, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-05] mmap1 failed\n");
        file_ng++;
        return;
    }
    //printf("- ret=%p\n", ret);
    // write & read private mapping on read only opened file
    char *ret2 =
        mmap((void *)0, 200, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (ret2 == MAP_FAILED) {
        printf("[F-05] mmap2 failed\n");
        file_ng++;
        munmap((void *)ret, 200);
        return;
    }
    //printf("- ret2=%p\n", ret2);
    for (int i = 0; i < 20; i++) {
        ret2[i] = 'a';
    }
    // printf("- set rets[0-20]='a'\n");
    int res = munmap(ret, 200);
    if (res == -1) {
        printf("[F-05] munmap1 failed\n");
        file_ng++;
        return;
    }
    res = munmap(ret2, 200);
    if (res == -1) {
        printf("[F-05] munmap2 failed\n");
        file_ng++;
        return;
    }
    printf("[F-05] ok\n");
    close(fd);
    file_ok++;
}

// [F-06] MMAPTOPを超えるサイズをマッピングした場合のテスト
// test 1: エラーになる
void file_exceed_size_test() {
    printf("\n[F-06] MMAPTOPを超えるサイズをマッピングした場合のテスト\n");
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("error: at open\n");
        file_ng++;
        return;
    }
    int size = 600 * 1024 * 1024;  // 600 MB
    void *ret = mmap((void *)0, size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 0);
    if (ret != MAP_FAILED) {
        printf("[F-06] failed\n");
        munmap(ret, size);
        close(fd);
        file_ng++;
        return;
    }
    perror(" error");
    close(fd);
    printf("[F-06] ok\n");
    file_ok++;
}

// [F-07] 連続したマッピングを行うテスト
// test 1: 50回の連続マッピングが成功する
void file_exceed_count_test() {
    printf("\n[F-07] 連続したマッピングを行うテスト\n");
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-07] failed\n");
        file_ng++;
        return;
    }
    int size = 4096;
    int count = 50;
    void *arr[50];
    int i = 0;
    for (; i < count; i++) {
        arr[i] = mmap((void *)0, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE, fd, 0);
        if (arr[i] == MAP_FAILED) {
            perror(" error");
            break;
        }
    }
    if (i == 50) {
        for (int j = 0; j < i; j++) {
            int ret = munmap(arr[j], size);
            if (ret == -1) {
                printf(
                    "[F-07] failed at %d munmap\n", j);
                file_ng++;
                return;
            }
        }
        printf("[F-07] ok\n");
        close(fd);
    } else {
        printf(
            "[F-07] failed at %d total mappings\n",
            i);
        file_ng++;
        for (int j = 0; j < i; j++) {
            munmap((void *)arr[j], size);
        }
        return;
    }
    file_ok++;
}

// [F-14] オフセットを指定したプライベートマッピングのテスト
// test 1: オフセットマッピングとファイル読み込みの結果は一致する
// test 2: マッピングアドレスへの書き込みとバッファへの書き込みの結果は一致する
void file_mapping_with_offset_test() {
    printf("\n[F-14] オフセットを指定したプライベートマッピングのテスト\n");
    write_readme(4296);
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("error: at open\n");
        file_ng++;
        return;
    }
    int size = 4296;
    // char buf[4096];
    int n = read(fd, buf, 4096);  // Move to offset 200
    if (n != 4096) {
        printf("[F-14] failed at read, n=%d\n",
               n);
        file_ng++;
        return;
    }
    n = read(fd, buf, size);
    if (n != (size - 4096)) {
        printf("[F-14] failed at read 2, n=%d\n",
               n);
        file_ng++;
        return;
    }
    // Offset is 4096
    volatile char *ret = (volatile char *)mmap((void *)0, (size - 4096), PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 4096);
    if (ret == (char *)-1) {
        printf("[F-14] failed 1\n");
        file_ng++;
        return;
    }
    if (my_strcmp(buf, ret, (size - 4096)) != 0) {
        printf("[F-14] failed 2\n");
        file_ng++;
        munmap((void *)ret, size);
        return;
    }
    for (int i = 0; i < 40; i++) {
        ret[i] = 'p';
        buf[i] = 'p';
    }
    if (my_strcmp(buf, ret, (size - 4096)) != 0) {
        printf("[F-14] failed 3\n");
        file_ng++;
        munmap((void *)ret, size);
        return;
    }
    int res = munmap((void *)ret,(size - 4096));
    if (res == -1) {
        printf("[F-14] failed 4\n");
        file_ng++;
        return;
    }
    close(fd);
    printf("[F-14] ok\n");
    file_ok++;
}

// [F-08] 空のファイルを共有マッピングした場合のテスト
// test 1: マッピングは成功する
// test 2: マッピングに書き込んだ結果はファイルに反映する
void file_empty_file_size_test() {
    printf("\n[F-08] 空のファイルを共有マッピングした場合のテスト\n");
    int fd = open(filename, O_TRUNC | O_RDWR, 0666);
    if (fd == -1) {
        printf("[F-08] at open\n");
        file_ng++;
        return;
    }
    volatile char *ret =
        (char *)mmap((void *)0, 32, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-08] at mmap\n");
        file_ng++;
        return;
    }

    for (int i = 0; i < 200; i++) {
        ret[i] = 'a';
    }
    /*
      printf("ret(%p): ", ret);
      char c;
      for (int i = 0; i < 32; i++) {
          c = (ret[i] < 0x20 || ret[i] > 0x7e) ? '.' : ret[i];
          printf("%c", c);
      }
      printf("\n");
    */
    int res = munmap(ret, 32);
    if (res == -1) {
        printf("[F-08] failed at munmap\n");
        file_ng++;
        return;
    }
    close(fd);
    // Check if data is correctly written into the file
    fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-08] at open file1\n");
        file_ng++;
        return;
    }
    char buf[32];
    int n = read(fd, buf, 32);
    if (n != 32) {
        printf("[F-08] at read\n");
        file_ng++;
        return;
    }

    /*
      char d = buf[0];
      printf("d='%c'\n", ((d < 0x20 || d > 0x7e) ? '.' : d));
      for (int j = 0; j < 200; j++) {
          if (buf[j] != d) printf("buf[%d]='%c'\n", j, buf[j]);
          break;
      }

      printf("buf: ");
      for (int i = 0; i < 200; i++) {
          c = (buf[i] < 0x20 || buf[i] > 0x7e) ? '.' : buf[i];
          printf("%c", c);
      }
      printf("\n");
      */

    for (int i = 0; i < 32; i++) {
        if (buf[i] != 'a') {
            printf("[F-08] at buf[%d]\n", i);
            file_ng++;
            return;
        }
    }
    printf("[F-08] ok\n");
    close(fd);
    file_ok++;
}

// [F-20] 共有マッピングでファイル容量より大きなサイズを指定した場合
// test 1: マッピングは成功する
// test 2: 容量以上の書き込みをしても問題ない（容量以降の書き込み結果の比較はしていない）
void file_exceeds_file_size_test() {
    printf("\n[F-20] 共有マッピングでファイル容量より大きなサイズを指定した場合\n");
    // README file size 2286 bytes but we are mapping 8000 bytes and then
    // writing upto 8000 bytes in it
    write_readme(2286);
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-20] failed at open file\n");
        file_ng++;
        return;
    }
    // char buf[3000];
    int n = read(fd, buf, 2286);
    if (n != 2286) {
        printf("[F-20] failed at read\n");
        file_ng++;
        return;
    }
    int size = 8000;
    volatile char *ret = (volatile char *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-20] failed at mmap\n");
        file_ng++;
        return;
    }
    for (int i = 0; i < size; i++) {
        ret[i] = 'a';
    }

    if (my_strcmp(ret, buf, 2286) != 0) {
        printf("[F-20] failed strcmp\n");
        file_ng++;
        munmap((void *)ret, size);
        return;
    }
    int res = munmap(ret, size);
    if (res == -1) {
        printf("[F-20] failed munmap\n");
        file_ng++;
        return;
    }
    printf("[F-20] ok\n");
    file_ok++;
}

// [F-21] write onlyファイルへのREAD/WRITE共有マッピングのテスト
// test 1: エラーになる
void file_mapping_on_wo_file_test() {
    printf("\n[F-21] write onlyファイルへのREAD/WRITE共有マッピングのテスト\n");
    int fd = open(filename, O_WRONLY);
    if (fd == -1) {
        printf("[F-21] failed at open file\n");
        file_ng++;
        return;
    }
    int size = 2000;
    char *ret = (char *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-21] ok\n");
        file_ok++;
        return;
    }
    printf("[F-21] failed\n");
    file_ng++;
    munmap((void *)ret, size);
}

// [F-11] ファイルに書き込み後にページキャッシュが変更されるかのテスト
// test 1: 先頭からマッピングしたデータはファイルに一致する
// test 2: オフセット位置に書き込んだファイルとオフセット指定でマッピングしたデータは一致する
void file_pagecache_coherency_test() {
    printf("\n[F-11] file backed mapping pagecache coherency test\n");
    write_readme(4296);
    int size = 100;
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-11] failed at open file\n");
        goto f11_bad;
    }
    // char buf[100];
    int n = read(fd, buf, size);
    if (n != size) {
        printf("[F-11] failed at read\n");
        goto f11_bad;
    }
    char *ret = (char *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-11] failed at mmap\n");
        goto f11_bad;
    }

    int j = my_strcmp(buf, ret, size);
    if (j != 0) {
      printf("file backed mapping pagecache coherency test failed 2: pos: %d, buf: %c, ret: %c\n",
    j, buf[j], ret[j]);
        goto f11_bad_unmap;
    }

    char a[100];
    for (int i = 0; i < 100; i++) a[i] = 'a';
    // Write some data to file at offset equal to size  'a' x 200
    n = write(fd, a, size);
    if (n != size) {
        printf("[F-11] failed at filewrite\n");
        close(fd);
        goto f11_bad_unmap;
    }
    close(fd);
    int fd2 = open(filename, O_RDWR);  // Again open file just to seek to start
    if (fd2 == -1) {
        printf("[F-11] failed at open file 2\n");
        goto f11_bad_unmap;
    }
    size = 4196;
    n = read(fd2, buf, size - 100);  // For the lseek: file offset = 4096
    if (n != (size - 100)) {
        printf("[F-11] failed at read 2\n");
        goto f11_bad_unmap;
    }
    n = read(fd2, buf, size - 4096);  // Read from the file at offset size: read 4097-4196, file offset = EOF
    if (n != size - 4096) {
        printf("[F-11] failed at read 3\n");
        close(fd2);
        goto f11_bad_unmap;
    }
    char *ret2 = (char *)mmap((void *)0, (size - 4096), PROT_READ | PROT_WRITE,
                              MAP_PRIVATE, fd2, 4096);
    if (ret2 == MAP_FAILED) {
        printf("[F-11] failed: at mmap 2\n");
        close(fd2);
        goto f11_bad_unmap;
    }
    if (my_strcmp(ret2, buf, size - 4096) != 0) {
        printf("[F-11] failed at strcmp\n");
        close(fd2);
        goto f11_bad_unmap2;
    }
    int res = munmap(ret, 100);
    if (res == -1) {
        printf("[F-11] failed at munmap\n");
        munmap(ret2, size);
        close(fd2);
        goto f11_bad_unmap2;
    }
    res = munmap(ret2, size);
    if (res == -1) {
        close(fd2);
        goto f11_bad;
    }
    printf("[F-11] ok\n");
    close(fd2);
    file_ok++;
    return;

f11_bad_unmap2:
    munmap((void *)ret2, size);
f11_bad_unmap:
    munmap((void *)ret, size);
f11_bad:
    file_ng++;
    printf("[F-11] failed\n");
    return;
}

// private file backed mapping with fork test
void file_private_with_fork_test() {
    printf("\n[F-12] file backed private mapping with fork test\n");
    int size = 200;
    char buf[200];

    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-12] failed at open file\n");
        file_ng++;
        return;
    }
    if (read(fd, buf, size) != size) {
        printf("[F-12] failed at read\n");
        file_ng++;
        return;
    }
    volatile char *ret =
        (volatile char *)mmap((void *)0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-12] failed: at mmap\n");
        close(fd);
        file_ng++;
        return;
    }
    int pid = fork();
    if (pid < 0) {
        printf("[F-12] failed: fork\n");
        munmap((void *)ret, size);
        close(fd);
        file_ng++;
        return;
    }
    int ng = 0;
    if (pid == 0) {
        for (int i = 0; i < 50; i++) {
            ret[i] = 'n';
        }
        // The mapping should not be same as we have edited the data
        if (my_strcmp(ret, buf, size) == 0) {
            printf("[F-12] failed at strcmp child\n");
            ng = 1;
        }
        exit(0);
    } else {
        int status;
        wait(&status);
        // As it is private mapping therefore it should be same as read
        if (my_strcmp(ret, buf, size) != 0) {
            printf("[F-12] failed at strcmp parent\n");
            file_ng++;
            munmap((void *)ret, size);
            return;
        }
        int res = munmap((void *)ret, size);
        if (res == -1) {
            printf("[F-12] failed at munmap\n");
            file_ng++;
            return;
        }
        if (ng) {
            file_ng++;
            return;
        }
        printf("[F-12] ok\n");
        file_ok++;
    }
}

// shared file backed mapping with fork test
void file_shared_with_fork_test() {
    printf("\n[F-13] file backed shared mapping with fork test\n");
    int size = 200;
    char buf[200];
    int status;

    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-13] failed at open file\n");
        file_ng++;
        return;
    }
    if (read(fd, buf, size) != size) {
        printf("[F-13] failed at read\n");
        file_ng++;
        return;
    }
    char *ret2 =
        mmap((void *)0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    int pid = fork();
    int ng = 0;
    if (pid == 0) {
        for (int i = 0; i < 50; i++) {
            ret2[i] = 'o';
        }
        if (my_strcmp(ret2, buf, size) == 0) {
            printf("[F-13] failed at strcmp child: ret2[0]=%c, buf[0]=%c\n", ret2[0], buf[0]);
            ng = 1;
        }
        exit(0);
    } else {
        wait(&status);
        // The data written in child process should persist here
        if (my_strcmp(ret2, buf, size) == 0) {
            printf("[F-13] failed at strcmp parent: ret2[0]=%c, buf[0]=%c\n", ret2[0], buf[0]);
            file_ng++;
            munmap(ret2, size);
            return;
        }
        int res = munmap(ret2, size);
        if (res == -1) {
            printf("[F-13] failed at munmap\n");
            file_ng++;
            return;
        }
        close(fd);
        // Check if data is written into file
        int fd2 = open(filename, O_RDWR);
        for (int i = 0; i < 50; i++) {
            buf[i] = 'o';
        }
        char buf2[200];
        if (read(fd2, buf2, size) != size) {
            printf("[F-13] failed at read 2\n");
            file_ng++;
            return;
        }
        if (my_strcmp(buf2, buf, size) != 0) {
            printf("[F-13] failed at strcmp 3\n");
            file_ng++;
            return;
        }
        if (ng) {
            file_ng++;
            return;
        }
        printf("[F-13] ok\n");
        close(fd2);
        file_ok++;
    }
}

// file backed mmap when the valid address is provided by user
void file_given_addr_test() {
    printf("\n[F-15] file backed valid provided address test\n");
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("error: file open\n");
        file_ng++;
        return;
    }
    char *ret = (char *)mmap((void *)(MMAPBASE + 0x1000), 200, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-15] failed at mmap\n");
        file_ng++;
        return;
    }
    int res = munmap(ret, 200);
    if (res == -1) {
        printf("[F-15] failed at munmap\n");
        file_ng++;
        return;
    }
    close(fd);
    printf("[F-15] ok\n");
    file_ok++;
}

// file backed mmap when provided address is less than MMAPBASE
// これはテスト自体が問題: エラーにはしない
void file_invalid_addr_test(void) {
    printf("\n[F-16] file backed invalid provided address test\n");
    write_readme(1000);
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-16] failed at open file\n");
        file_ng++;
        return;
    }
    char *ret = (char *)mmap((void *)(MMAPBASE - 0x1000), 200, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-16] failed\n");
        file_ng++;
        return;
    }
    printf("[F-16] ok\n");
    munmap(ret, 200);
    close(fd);
    file_ok++;
}

// [F-17] 指定されたアドレスが既存のマッピングアドレスと重なる場合
// test 1: アドレス指定(MMAPBASE + 0x1000)で8000バイトマッピング
// test 2: 1と重なるアドレス指定(MMAPBASE + 0x1000)で200バイトマッピング
void file_overlap_given_addr_test() {
    printf("\n[F-17] 指定されたアドレスが既存のマッピングアドレスと重なる場合\n");
    write_readme(8000);
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-17] failed at open file\n");
        file_ng++;
        return;
    }
    char *ret = (char *)mmap((void *)(MMAPBASE + 0x1000), 8000, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED) {
        printf(
            "[F-17] failed at first mmap\n");
        file_ng++;
        return;
    }
    //printf("- ret=%p\n", ret);
    char *ret2 = (char *)mmap((void *)(MMAPBASE + 0x1000), 200, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE, fd, 0);
    if (ret2 == MAP_FAILED || ret2 == (void *)(MMAPBASE + 0x1000)) {
        printf("[F-17] failed: at second mmap: addr: 0x%p\n", ret2);
        munmap(ret, 10000);
        file_ng++;
        return;
    }
    //printf("- ret2=%p\n", ret2);
    int res = munmap(ret, 8000);
    if (res == -1) {
        printf("[F-17] failed at first munmap\n");
        file_ng++;
        munmap(ret2, 200);
        return;
    }
    res = munmap(ret2, 200);
    if (res == -1) {
        printf("[F-17] failed at second munmap\n");
        file_ng++;
        return;
    }
    printf("[F-17] ok\n");
    close(fd);
    file_ok++;
}

// [F-18] ２つのマッピングの間のアドレスを指定したマッピングのテスト
// test 1: addr指定せずに1000バイトのマッピング
// test 2: addr指定(MMAPBASE + 0x3000)して200バイトのマッピング
// test 3: addr指定(MMAPBASE + 0x100)して1000バイトのマッピング
void file_intermediate_given_addr_test() {
    printf("\n[F-18] ２つのマッピングの間のアドレスを指定したマッピングのテスト\n");
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-18] failed at open file\n");
        file_ng++;
        return;
    }
    char *ret = (char *)mmap((void *)0, 1000, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED) {
        printf("[F-18] failed at first mmap\n");
        file_ng++;
        return;
    }
    //printf("ret=%p\n", ret);
    char *ret2 = (char *)mmap((void *)(MMAPBASE + 0x3000), 200, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE, fd, 0);
    if (ret2 == MAP_FAILED) {
        printf("[F-18] failed at second mmap\n");
        munmap(ret, 1000);
        file_ng++;
        return;
    }
    //printf("ret2=%p\n", ret2);
    char *ret3 = (char *)mmap((void *)(MMAPBASE + 0x100), 1000, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE, fd, 0);
    if (ret3 == MAP_FAILED) {
        printf("[F-18] failed at third mmap\n");
        munmap(ret, 1000);
        file_ng++;
        return;
    }
    if (ret3 != ret + 0x1000) {
        printf("[F-18] expect: %p, mapped: %p\n", ret + 0x1000, ret3);
        munmap(ret, 1000);
        munmap(ret2, 200);
        if (ret3 != MAP_FAILED)
            munmap(ret3, 1000);
        file_ng++;
        return;
    }
    //printf("ret3=%p\n", ret3);
    int res = munmap(ret, 1000);
    if (res == -1) {
        printf("[F-18] failed at first munmap\n");
        munmap(ret2, 200);
        munmap(ret3, 1000);
        file_ng++;
        return;
    }
    res = munmap(ret2, 200);
    if (res == -1) {
        printf("[F-18] failed at second munmap\n");
        munmap(ret3, 1000);
        file_ng++;
        return;
    }
    res = munmap(ret3, 1000);
    if (res == -1) {
        printf("[F-18] failed: at third munmap\n");
        file_ng++;
        return;
    }
    close(fd);
    printf("[F-18] ok\n");
    file_ok++;
}

// [F-19] ２つのマッピングの間に不可能なアドレスを指定した場合
// test 1: アドレス指定なしで1000バイトマッピング
// test 2: アドレス指定(MMAPBASE + 0x3000)して200バイトマッピング
// test 3: アドレス指定(MMAPBASE + 0x100)して10000バイトマッピング
void file_intermediate_given_addr_not_possible_test() {
    printf(
        "\n[F-19] ２つのマッピングの間に不可能なアドレスを指定した場合\n");
    write_readme(4296);
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("[F-19] failed at open file\n");
        file_ng++;
        return;
    }
    char *ret = (char *)mmap((void *)0, 1000, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE, fd, 0);
    printf("ret =%p\n", ret);
    if (ret == MAP_FAILED) {
        printf("[F-19]failed at first mmap\n");
        file_ng++;
        return;
    }
    char *ret2 = (char *)mmap((void *)(MMAPBASE + 0x3000), 200, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE, fd, 0);
    printf("ret2=%p\n", ret2);
    if (ret2 == MAP_FAILED) {
        printf("[F-19] failed at second mmap\n");
        file_ng++;
        munmap(ret, 1000);
        return;
    }
    char *ret3 = (char *)mmap((void *)(MMAPBASE + 0x100), 10000, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE, fd, 0);
    printf("ret3=%p\n", ret3);
    if (ret3 == MAP_FAILED) {
        printf("[F-19] failed at third mmap\n");
        file_ng++;
        munmap(ret, 1000);
        munmap(ret2, 200);
        return;
    }
    if (ret3 == (void *)(MMAPBASE + 0x100)) {
        printf("[F-19] failed at third mmap with wrong addr\n");
        file_ng++;
        munmap(ret, 1000);
        munmap(ret2, 200);
        return;
    }
    //printf("ret=%p, ret2=%p, ret3=%p\n", ret, ret2, ret3);
    munmap(ret, 1000);
    munmap(ret2, 200);
    munmap(ret3, 10000);
    close(fd);
    printf("[F-19]  ok\n");
    file_ok++;
}

// <!! ----------- Anonymous mappings test ---------------------- !!>

// Missing flags Test: Missing MAP_PRIVATE or MAP_SHARED in flags
void anon_missing_flags_test(void) {
    printf("\n[A-06] anonymous missing flags test\n");
    int size = 10000;
    // Missing MAP_PRIVATE or MAP_SHARED flag
    int *p1 = (int *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS, -1, 0);
    if (p1 != MAP_FAILED) {
        printf("[A-06] failed at mmap\n");
        munmap((void *)p1, size);
        anon_ng++;
        return;
    }
    printf("[A-06] ok\n");
    anon_ok++;
}

// anonymous mapping when size exceeds KERNBASE
void anon_exceed_size_test(void) {
    printf("\n[A-08] anonymous exceed mapping size test\n");
    unsigned long size = (1UL << 48);  // USERTOP
    char *p1 = (char *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                             MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (p1 != MAP_FAILED) {
        printf("[A-08] failed at mmap: p1=%p\n", p1);
        munmap((void *)p1, size);
        anon_ng++;
        return;
    }
    printf("[A-08] ok\n");
    anon_ok++;
}

// mmap when provided size is 0
void anon_zero_size_test(void) {
    printf("\n[A-09] anonymous zero size mapping test\n");
    char *p1 = (char *)mmap((void *)0, 0, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p1 != MAP_FAILED) {
        printf("[A-09] failed\n");
        munmap((void *)p1, 0);
        anon_ng++;
        return;
    }
    printf("[A-09] ok\n");
    anon_ok++;
}

// anonymous mapping count test when it exceeds 32
void anon_exceed_count_test(void) {
    printf("\n[A-07] anonymous exceed mapping count test\n");
    int size = 5096;
    int count = 50;
    char *arr[50];
    int i = 0;
    long error;

    for (; i < count; i++) {
        void *p1 = mmap((void *)0, size, PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        arr[i] = (char *)p1;
        //printf("arr[%d]=%p\n", i, arr[i]);
        if (p1 == MAP_FAILED) {
            break;
        }
    }
    if (i == 50) {  // 32 = VMASIZE
        for (int j = 0; j < i; j++) {
            error = munmap((void *)arr[j], size);
            if (error < 0) {
                printf("[A-07] failed at %d munmap\n", j);
                anon_ng++;
                return;
            }
        }
        printf("[A-07] ok\n");
    } else {
        printf("[A-07] failed %d total mappings\n", i);
        for (int j = 0; j < i; j++) {
            error = munmap((void *)arr[j], size);
            if (error < 0) printf("arr[%d]=%p failed munmaping\n", j, arr[j]);
        }
        anon_ng++;
        return;
    }
}

// Simple private anonymous mapping test with maping having both read and write
// permission and size greater than two pages
void anon_private_test() {
    printf("\n[A-01] anonymous private mapping test\n");
    int size = 10000;
    long error;

    volatile int *p1 = (volatile int *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (p1 == MAP_FAILED) {
        printf("[A-01] failed at mmap\n");
        anon_ng++;
        return;
    }
    //volatile int *intp = (volatile int *)p1;
    //printf("- p1=%p\n", p1);
    // これを消すとmunmapの第1引数がret=0x0となりエラー
    for (int i = 0; i < size / 4; i++) {
        p1[i] = i;
    }
    printf("p[0]=%d, p[2499]=%d\n", p1[0], p1[2499]);
    error = munmap((void *)p1, size);
    if (error < 0) {
        printf("[A-01] failed at munmap\n");
        anon_ng++;
        return;
    }
    printf("[A-01] ok\n");
    anon_ok++;
}

// Shared mapping test
void anon_shared_test() {
    printf("\n[A-02] anonymous shared mapping test\n");
    size_t size = 10000;
    long error;

    volatile int *p1 = (volatile int *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANONYMOUS, -1,
                           0);  // Shared mapping
    if (p1 == MAP_FAILED) {
        printf("[A-02] failed mmap\n");
        anon_ng++;
        return;
    }
    int pid = fork();
    if (pid < 0) {
        printf("[A-02] failed fork\n");
        anon_ng++;
        return;
    }
    if (pid == 0) {
        for (int i = 0; i < size / 4; i++) {
            p1[i] = i;
        }
        exit(0);
    } else {
        int status;
        wait(&status);
        msync(p1, size, MS_ASYNC);
        for (int i = 0; i < size / 4; i++) {
            if (p1[i] != i) {
                printf("[A-02] p1[%d]: %d != %d\n", i, p1[i], i);
                munmap((void *)p1, size);
                anon_ng++;
                return;
            }
        }
        error = munmap((void *)p1, size);
        if (error < 0) {
            printf("[A-02] failed munmap\n");
            anon_ng++;
            return;
        }
        printf("[A-02] ok\n");
        anon_ok++;
    }
}

// Shared mapping test with multiple forks
void anon_shared_multi_fork_test() {
    printf("\n[A-04] anonymous shared mapping with multiple forks test\n");
    int size = 1000;
    int status;

    char *p1 = mmap((void *)0, size, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);  // Shared mapping
    if (p1 == MAP_FAILED) {
        printf("[A-04] failed at mmap\n");
        anon_ng++;
        return;
    }
    char data[1000];
    for (int i = 0; i < size; i++) {
        data[i] = 'r';
    }
    int pid = fork();
    int ng = 0;
    if (pid == -1) {
        printf("[A-04] at fork 1\n");
        munmap(p1, size);
        anon_ng++;
        return;
    }
    if (pid == 0) {  // 1st fork Child Process
        for (int i = 0; i < size; i++) {
            p1[i] = 'r';
        }
        int pid2 = fork();
        if (pid2 == -1) {
            printf("[A-04] failed at fork 2\n");
            anon_ng++;
            munmap((void *)p1, size);
            exit(1);
        }
        if (pid2 == 0) {  // 2nd fork Child Process
            if (my_strcmp(data, p1, size) != 0) {
                printf("[A-04]  failed at strcmp fork 2 child\n");
                ng = 1;
            }
            int pid3 = fork();
            if (pid3 == -1) {
                printf("[A-04] failed at fork 3\n");
                anon_ng++;
                munmap((void *)p1, size);
                exit(1);
            }
            if (pid3 == 0) {  // 3rd fork Child Process
                if (my_strcmp(data, p1, size) != 0) {
                    printf("[A-04] failed at strcmp fork 3 child\n");
                    ng = 1;
                }
                exit(1);
            } else {  // 3rd fork Parent Process
                wait(&status);
                if (my_strcmp(data, p1, size) != 0) {
                    printf("[A-04] failed at strcmp fork 3 parent\n");
                    ng = 1;
                }
                exit(1);
            }
        } else {  // 2nd fork Parent Process
            wait(&status);
            if (my_strcmp(data, p1, size) != 0) {
                printf("[A-04] failed at strcmp fork 2 parent\n");
                ng = 1;
            }
            exit(1);
        }
        exit(1);
    } else {  // 1st fork Parent process
        wait(&status);
        if (my_strcmp(data, p1, size) != 0) {
            printf("[A-04] failed at strcmp fork 1 parent\n");
            ng = 1;
        }
        int res = munmap((void *)p1, size);
        if (res == -1) {
            printf("[A-04] failed at munmap fork 1 parent5\n");
            anon_ng++;
            return;
        }
        if (ng) {
            anon_ng++;
            return;
        }
        printf("[A-04] ok\n");
        anon_ok++;
    }
}

// Private mapping with fork
void anon_private_fork_test() {
    printf("\n[A-03] anonymous private mapping with fork test\n");
    char temp[200];
    int status;

    for (int i = 0; i < 200; i++) {
        temp[i] = 'a';
    }
    int size = 200;
    char *p1 = mmap((void *)0, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);  // Shared mapping
    if (p1 == MAP_FAILED) {
        printf("[A-03] failed at mmap\n");
        anon_ng++;
        return;
    }
    int pid = fork();
    if (pid == 0) {
        for (int i = 0; i < size; i++) {
            p1[i] = 'a';
        }
        exit(0);
    } else {
        wait(&status);
        if (my_strcmp(temp, p1, size) == 0) {
            printf("[A-03] failed at strcmp parent\n");
            anon_ng++;
            munmap((void *)p1, size);
            return;
        }
        munmap((void *)p1, size);
        printf("[A-03] ok\n");
        anon_ok++;
    }
}

// fork syscall with anonymous mapping test
void anon_private_shared_fork_test() {
    printf(
        "\n[A-05] anonymous private & shared mapping together with fork "
        "test\n");
    int size = 200;
    char data1[200], data2[200];
    int status;
    long error;

    for (int i = 0; i < size; i++) {
        data1[i] = 'a';
        data2[i] = 'r';
    }
    char *p1 = mmap((void *)0, size, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);  // Private mapping
    if (p1 == MAP_FAILED) {
        printf("[A-05] failed at mmap 1\n");
        anon_ng++;
        return;
    }
    char *p2 = mmap((void *)0, size, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);  // Shared mapping
    if (p2 == MAP_FAILED) {
        printf("[A-05] failed at mmap 2\n");
        anon_ng++;
        munmap((void *)p1, size);
        return;
    }
    int pid = fork();
    if (pid == -1) {
        printf("[A-05] failed at fork\n");
        anon_ng++;
        munmap((void *)p1, size);
        munmap((void *)p2, size);
        return;
    }
    int ng = 0;
    if (pid == 0) {
        for (int i = 0; i < size; i++) {
            p1[i] = 'a';
        }
        for (int i = 0; i < size; i++) {
            p2[i] = 'r';
        }
        exit(1);
    } else {
        wait(&status);
        // Private mapping
        if (my_strcmp(p1, data1, size) == 0) {
            printf("[A-05] failed at strcmp private\n");
            anon_ng++;
            munmap((void *)p1, size);
            munmap((void *)p1, size);
            return;
        }
        // Shared mapping
        if (my_strcmp(p2, data2, size) != 0) {
            printf("[A-05] failed at strcmp share\n");
            anon_ng++;
            munmap((void *)p1, size);
            munmap((void *)p2, size);
            return;
        }
        error = munmap((void *)p1, size);
        if (error < 0) {
            printf("[A-05] failed at munmap 1\n");
            anon_ng++;
            munmap((void *)p2, size);
            return;
        }
        error = munmap((void *)p2, size);
        if (error < 0) {
            printf("[A-05] failed at munmap 2\n");
            anon_ng++;
            return;
        }
        printf("[A-05] ok\n");
        anon_ok++;
    }
}

// mmap when the valid address is provided by user
void anon_given_addr_test() {
    printf("\n[A-10] anonymous valid provided address test\n");
    char *p1 = (char *)mmap((void *)(MMAPBASE + 0x1000), 200, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p1 == MAP_FAILED) {
        printf("[A-10] failed: at mmap\n");
        anon_ng++;
        return;
    }
    int res = munmap((void *)p1, 200);
    if (res == -1) {
        printf("[A-10] failed at munmap\n");
        anon_ng++;
        return;
    }
    printf("[A-10] ok\n");
    anon_ok++;
}

// mmap when provided address is less than MMAPBASE
// このテストは無効: MMAPBASE以下のアドレスもmmap可能にしたため
void anon_invalid_addr_test(void) {
    printf("\n[A-11] anonymous invalid provided address test\n");
/*
    char *p1 = (char *)mmap((void *)0x50001000, 200, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p1 != MAP_FAILED) {
        printf("[A-11] failed at mmap\n");
        munmap((void *)p1, 200);
        anon_ng++;
        return;
    }
*/
    printf("[A-11] ok: not running because of an invalid test\n");
    anon_ok++;
}

// test when the address is provided by user and it overlaps with existing
// address
void anon_overlap_given_addr_test() {
    long error;

    printf("\n[A-12] anonymous overlapping provided address test\n");
    char *p1 = (char *)mmap((void *)(MMAPBASE + 0x1000), 10000, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p1 == MAP_FAILED) {
        printf("[A-12] failed: at first mmap\n");
        anon_ng++;
        return;
    }
    char *p2 = (char *)mmap((void *)(MMAPBASE + 0x1000), 200, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p2 == MAP_FAILED || p2 == (void *)(MMAPBASE + 0x1000)) {
        printf("[A-12] failed at second mmap\n");
        munmap((void *)p1, 10000);
        anon_ng++;
        return;
    }
    error = munmap((void *)p1, 10000);
    if (error < 0) {
        printf("[A-12] failed at first munmap\n");
        anon_ng++;
        munmap((void *)p2, 200);
        return;
    }
    error = munmap((void *)p2, 200);
    if (error < 0) {
        printf("[A-12] failed: at first munmap\n");
        anon_ng++;
        return;
    }
    printf("[A-12] ok\n");
    anon_ok++;
}

// test when the mapping is possible between two mappings
void anon_intermediate_given_addr_test() {
    long error;

    printf("\n[A-13] anonymous intermediate provided address test\n");
    char *p1 = (char *)mmap((void *)0, 1000, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //printf("p1=%p\n", p1);
    if (p1 == MAP_FAILED) {
        printf("[A-13] failed: failed at first mmap\n");
        anon_ng++;
        return;
    }

    char *p2 = (char *)mmap((void *)(MMAPBASE + 0x3000), 200, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //printf("p2=%p\n", p2);
    if (p2 == MAP_FAILED) {
        printf("[A-13] failed at second mmap\n");
        munmap((void *)p1, 1000);
        anon_ng++;
        return;
    }

    char *p3 = (char *)mmap((void *)(MMAPBASE + 0x100), 1000, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //printf("p3=%p\n", p3);
    if (p3 == MAP_FAILED) {
        printf("[A-13] failed at third mmap\n");
        munmap((void *)p1, 1000);
        munmap((void *)p2, 1000);
        anon_ng++;
        return;
    }
    if (p3 != (p1 + 0x1000)) {
        printf("[A-13] expect: %p, mapped: %p\n", (void *)(MMAPBASE + 0x1000), p3);
        munmap((void *)p1, 1000);
        munmap((void *)p2, 200);
        anon_ng++;
        return;
    }
    error = munmap((void *)p1, 1000);
    if (error < 0) {
        printf("[A-13] failed at first munmap\n");
        munmap((void *)p2, 200);
        munmap((void *)p3, 1000);
        anon_ng++;
        return;
    }
    error = munmap((void *)p2, 200);
    if (error < 0) {
        printf("[A-13] failed at second munmap\n");
        munmap((void *)p3, 1000);
        anon_ng++;
        return;
    }
    error = munmap((void *)p3, 1000);
    if (error < 0) {
        printf("[A-13] failed at third munmap\n");
        anon_ng++;
        return;
    }
    printf("[A-13] ok\n");
    anon_ok++;
}

// mmap when the mapping is not possible between two mappings
void anon_intermediate_given_addr_not_possible_test() {
    printf(
        "\n[A-14] anonymous intermediate provided address not possible test\n");
    char *p1 = (char *)mmap((void *)0, 1000, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p1 == MAP_FAILED) {
        printf("[A-14] failed at first mmap\n");
        anon_ng++;
        return;
    }
    char *p2 = (char *)mmap((void *)(MMAPBASE + 0x3000), 200, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p2 == MAP_FAILED) {
        printf("[A-14] failed at second mmap\n");
        anon_ng++;
        munmap((void *)p1, 1000);
        return;
    }
    char *p3 = (char *)mmap((void *)(MMAPBASE + 0x100), 10000, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p3 == MAP_FAILED) {
        printf("[A-14] failed at second mmap\n");
        anon_ng++;
        munmap((void *)p1, 1000);
        munmap((void *)p2, 200);
        return;
    }
    if (p3 == (void *)(MMAPBASE + 0x100)) {
        printf("[A-14] failed at third mmap\n");
        anon_ng++;
        munmap((void *)p1, 1000);
        munmap((void *)p2, 200);
        return;
    }
    munmap((void *)p1, 1000);
    munmap((void *)p2, 200);
    munmap((void *)p3, 10000);
    printf("[A-14] ok\n");
    anon_ok++;
}

// <!! ------------------Other MMAP Tests ------------- !!>

// When there is only read permission on mapping but user tries to write
void mmap_write_on_ro_mapping_test() {
    printf("\n[O-02] write on read only mapping test\n");
    int status;

    int pid = fork();
    if (pid == -1) {
        printf("[O-02] failed at fork\n");
        other_ng++;
        return;
    }
    if (pid == 0) {
        int size = 10000;
        int *ret = (int *)mmap((void *)0, size, PROT_READ,
                               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (ret == MAP_FAILED) {
            printf("[O-02] failed at mmap child\n");
            other_ng++;
            exit(1);
        }
        /*  ここで例外発生
            for (int i = 0; i < size / 4; i++) {
              ret[i] = i;
            }
            // If the memory access allowed then test should failed
            printf("write on read only mapping test failed 3\n");
            other_ng++;
        */
        munmap(ret, size);
        exit(0);
    } else {
        wait(&status);
        printf("[O-02] ok\n");
        other_ok++;
    }
}

// Munmap only partial size of the mapping test
void munmap_partial_size_test() {
    printf("\n[O-01] munmap only partial size test\n");
    int size = 10000;
    long error;

    int *p1 = (int *)mmap((void *)0, size, PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (p1 == MAP_FAILED) {
        printf("[O-01] failed at mmap\n");
        other_ng++;
        return;
    }
    for (int i = 0; i < size / 4; i++) {
        p1[i] = i;
    }
    // Munmap only first page
    error = munmap((void *)p1, 5);
    if (error < 0) {
        printf("[O-01] failed at munmap\n");
        other_ng++;
        munmap((void *)p1, size);
        return;
    }
    // Check if next page is still accessible or not
    for (int i = 1024; i < size / 4; i++) {
        p1[i] = i * 2;
    }
    error = munmap((void *)p1 + 4096, size);
    if (error < 0) {
        printf("[O-01] failed at munmap 2\n");
        other_ng++;
        return;
    }
    printf("[O-01] ok\n");
    other_ok++;
}

// None permission on mapping test
void mmap_none_permission_test() {
    printf("\n[O-03] none permission on mapping test\n");
    int status;

    int pid = fork();
    if (pid == -1) {
        printf("[O-03] failed at fork\n");
        other_ok++;
        return;
    }
    if (pid == 0) {
        int size = 10000;
        char *p1 = (char *)mmap((void *)(MMAPBASE + 0x3000), size, PROT_NONE,
                                 MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (p1 == MAP_FAILED) {
            printf("[O-03] failed mmap\n");
            other_ng++;
            exit(1);
        }
        /*
            printf("p1=%p\n", p1);

            for (int i = 0; i < size / 4; i++) {
              p1[i] = i;
            }
            // If the memory access allowed then test should failed
            printf("none permission on mapping test failed 3\n");
            other_ng++;
        */
        munmap((void *)p1, size);
        exit(0);
    } else {
        wait(&status);
        printf("[O-03] ok\n");
        other_ok++;
    }
}

// To test MAP_FIXED flag with valid address
void mmap_valid_map_fixed_test() {
    printf("\n[O-04] mmap valid address map fixed flag test\n");
    long error;

    char *p1 = mmap((void *)(MMAPBASE + 0x1000), 200, PROT_WRITE | PROT_READ,
                     MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p1 == MAP_FAILED) {
        printf("[O-04] failed at mmap\n");
        other_ng++;
        return;
    }
    error = munmap((void *)p1, 200);
    if (error < 0) {
        printf("[O-04] failed at munmap\n");
        other_ng++;
        return;
    }
    printf("[O-04] ok\n");
    other_ok++;
}

// To test MAP_FIXED flag with invalid addresses
void mmap_invalid_map_fixed_test() {
    printf("\n[O-05] mmap invalid address map fixed flag test\n");
    // When the address is less than MMAPBASE（これは現在は無効）
/*
    char *p1 = mmap((void *)(MMAPBASE - 0x2000), 200, PROT_WRITE | PROT_READ,
                     MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("p1=%p\n", p1);
    if (p1 != MAP_FAILED) {
        printf("[O-05] failed at mmap 1\n");
        munmap((void *)p1, 200);
        other_ng++;
        return;
    }
*/
    // When the address is not page aligned
    char *p2 = mmap((void *)(MMAPBASE + 0x100), 200, PROT_WRITE | PROT_READ,
                      MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //printf("p2=%p\n", p2);
    if (p2 != MAP_FAILED) {
        printf("[O-05] failed at mmap 2\n");
        other_ng++;
        return;
    }
    // Mapping is not possible because other mapping already exists at provided
    // address (MMAPBASEはdash)
    char *p3 = mmap((void *)MMAPBASE, 200, PROT_WRITE | PROT_READ,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //printf("p3=%p\n", p3);
    if (p3 == MAP_FAILED) {
        printf("[O-05] failed mmap 3\n");
        other_ng++;
        return;
    }
    char *p4 = mmap(p3, 200, PROT_WRITE | PROT_READ,
                      MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //printf("p4=%p\n", p4);
    if (p4 != MAP_FAILED) {
        printf("[O-05] failed mmap 4\n");
        munmap((void *)p3, 200);
        other_ng++;
        return;
    }
    munmap((void *)p3, 200);
    printf("[O-05] test ok\n");
    other_ok++;
}
