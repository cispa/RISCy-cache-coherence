#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "module/loongarch64_privileged.h"

int main() {
    // open the module
    int fd = open(MODULE_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open module: %s\n", MODULE_DEVICE_PATH);
        return -1;
    }

    int val = 0;
    int ret = ioctl(fd, MODULE_IOCTL_CMD_FLUSH, &val);
    printf("Flushing ret:  %d\n", ret);

    close(fd);
}
