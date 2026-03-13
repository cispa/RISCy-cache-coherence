#include "loongarch64_privileged.h"
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/version.h>

MODULE_AUTHOR("Fabian Thomas");
MODULE_DESCRIPTION("loongarch64_privileged Kernel Module");
MODULE_LICENSE("GPL");

#define TAG "[loongarch64_privileged-module] "

/* https://loongson.github.io/LoongArch-Documentation/LoongArch-Vol1-EN.html#overview-of-privilege-instructions */
#define flush(p) asm volatile("cacop 0b10011, %0, 0\n" ::"r"(p));
#define iflush(p) asm volatile("cacop 0b10000, %0, 0\n" ::"r"(p));

static int device_open(struct inode *inode, struct file *file) {
  /* Lock module */
  try_module_get(THIS_MODULE);
  return 0;
}

static int device_release(struct inode *inode, struct file *file) {
  /* Unlock module */
  module_put(THIS_MODULE);
  return 0;
}

static long device_ioctl(struct file *file, unsigned int ioctl_num,
                         unsigned long ioctl_param) {
  switch (ioctl_num) {
  case MODULE_IOCTL_CMD_FLUSH: {
      flush(ioctl_param);
      return 0;
  }
  case MODULE_IOCTL_CMD_IFLUSH: {
      iflush(ioctl_param);
      return 0;
  }
  default:
    return -1;
  }

  return 0;
}

static struct file_operations f_ops = {.unlocked_ioctl = device_ioctl,
                                       .open = device_open,
                                       .release = device_release};

static struct miscdevice misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MODULE_DEVICE_NAME,
    .fops = &f_ops,
    .mode = S_IRWXUGO,
};

int init_module(void) {
  int r;

  /* Register device */
  r = misc_register(&misc_dev);
  if (r != 0) {
    printk(KERN_ALERT TAG "Failed registering device with %d\n", r);
    return 1;
  }

  printk(KERN_INFO TAG "Loaded.\n");

  return 0;
}

void cleanup_module(void) {
  misc_deregister(&misc_dev);

  printk(KERN_INFO TAG "Removed.\n");
}
