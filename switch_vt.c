#include <fcntl.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int vt = 0;
    if (argc < 2) {
        fprintf(stderr, "Usage: switch_vt <vt>\n");
        return -1;
    }
    char* endptr;
    vt = strtol(argv[1], &endptr, 10);
    if (endptr == argv[1]) {
        fprintf(stderr, "vt sould be a number\n");
        return -1;
    }

    int fd = open("/dev/tty0", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "open tty0 fail\n");
        return -1;
    }
    int ret = ioctl(fd, VT_ACTIVATE, vt);
    if (-1 == ret) {
        fprintf(stderr, "active vt failed\n");
        return -1;
    }
    close(fd);
    return 0;
}
