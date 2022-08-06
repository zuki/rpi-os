#include <stdio.h>
#include <sys/mount.h>

int
main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage: mount dev, path, fs-type...\n");
        return 1;
    }

    if (mount(argv[1], argv[2], argv[3], 0UL, (char *)0) < 0) {
        printf("mount: failed to mouting device\n");
        return 1;
    }

    return 0;
}
