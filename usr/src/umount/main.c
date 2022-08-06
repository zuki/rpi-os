#include <stdio.h>
#include <sys/mount.h>

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: umount path...\n");
        return 1;
    }

    if (umount(argv[1]) < 0) {
        printf("umount: failed to unmouting device\n");
        return 1;
    }

    return 0;
}
