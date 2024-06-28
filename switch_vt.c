#include <fcntl.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <poll.h>

int main(void) {
    int fd = open("/dev/tty0", O_RDWR);
    assert(fd);
    int ret = ioctl(fd, VT_ACTIVATE, 4);
    assert(ret!=-1);
    ret = ioctl(fd, VT_WAITACTIVE, 2);
    assert(ret != -1);
    close(fd);
    return 0;
}
