#ifndef _MODULE_H
#define _MODULE_H

#include <stddef.h>

#define MODULE_DEVICE_NAME "loongarch64_privileged"
#define MODULE_DEVICE_PATH "/dev/" MODULE_DEVICE_NAME


#define MODULE_IOCTL_MAGIC_NUMBER (long)0x034000

#define MODULE_IOCTL_CMD_FLUSH \
  _IOR(MODULE_IOCTL_MAGIC_NUMBER, 1, size_t)
#define MODULE_IOCTL_CMD_IFLUSH \
  _IOR(MODULE_IOCTL_MAGIC_NUMBER, 2, size_t)

#endif
