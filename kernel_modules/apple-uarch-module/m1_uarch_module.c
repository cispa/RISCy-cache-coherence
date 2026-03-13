#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/bits.h>
#include <asm/sysreg.h>

#include "m1_uarch_module.h"

MODULE_AUTHOR("Lorenz Hetterich");
MODULE_DESCRIPTION("Microarchitectural Utilities for the Apple M1 Processor");
MODULE_LICENSE("GPL");

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


static int is_pcore(void) {
    uint64_t value;
    asm volatile(
        "mrs %0, MPIDR_EL1" : "=r" (value)
    );
    
    return (value & (1ull << 16ull)) != 0;
}

#ifdef APPLE_M2

//see https://stackoverflow.com/a/77821559
static void enable_counter(void* ignored) {
    printk(KERN_INFO "[m1-uarch] Enabling counters for M2.\n");

    int pmc_counter = 0;

    //configure
    u64 val, user_bit, kernel_bit;
    user_bit = BIT(get_bit_offset(pmc_counter, PMCR1_COUNT_A64_EL0_0_7));
    kernel_bit = BIT(get_bit_offset(pmc_counter, PMCR1_COUNT_A64_EL1_0_7));


    val = read_sysreg_s(SYS_IMP_APL_PMCR1_EL1);
    val |= user_bit;
    val |= kernel_bit;
    write_sysreg_s(val, SYS_IMP_APL_PMCR1_EL1);
    isb();

    //enable in user space
    write_sysreg_s(0xff | 0x40000000ull, sys_reg(3, 1, 15, 0, 0));
    isb();

    //start
    val = read_sysreg_s(SYS_IMP_APL_PMCR0_EL1);
    val &= ~(PMCR0_IMODE | PMCR0_IACT);
    val |= FIELD_PREP(PMCR0_IMODE, PMCR0_IMODE_FIQ);
    write_sysreg_s(val, SYS_IMP_APL_PMCR0_EL1);
    isb();
}

#else

static void enable_counter(void* ignored) {
    u64 val;
    (void) ignored;
    asm volatile("mrs %0, s3_1_c15_c1_0" : "=r" (val));
    val |= (uint64_t)0xffffffffffffffff;
    asm volatile("isb");
    asm volatile("msr s3_1_c15_c1_0, %0" : : "r" (val));
    asm volatile("mrs %0, s3_1_c15_c0_0" : "=r" (val));
    val |= (uint64_t)0xffffffffffffffff;
    asm volatile("isb");
    asm volatile("msr s3_1_c15_c0_0, %0" : : "r" (val));
}

#endif

static void enable_flushing(void* ignored) {
    u64 val;
    (void) ignored;
    if(is_pcore()) {
        asm volatile("mrs %0, S3_0_c15_c4_0" : "=r" (val));
        // ARM64_REG_HID4_DisDcMVAOps (bit 11) must be 0
        // ARM64_REG_HID4_DisDcSWL2Ops (bit 44) must be 0
        val &= 0xffffffffffffffffull ^ (1ull << 11ull) ^ (1ull << 44ull);
        asm volatile("isb");
        asm volatile("msr S3_0_c15_c4_0, %0" :: "r" (val));
    } else {
        asm volatile("mrs %0, S3_0_c15_c4_1" : "=r" (val));
        // ARM64_REG_HID4_DisDcMVAOps (bit 11) must be 0
        // ARM64_REG_HID4_DisDcSWL2Ops (bit 44) must be 0
        val &= 0xffffffffffffffffull ^ (1ull << 11ull) ^ (1ull << 44ull);
        asm volatile("isb");
        asm volatile("msr S3_0_c15_c4_1, %0" :: "r" (val));
    
    }
}

static long device_ioctl(struct file *file, unsigned int ioctl_num,
                         unsigned long ioctl_param) {
  switch(ioctl_num) {
  case CMD_SET_COUNTER_ENABLED: {
      if(ioctl_param) {
          // enable counter
          on_each_cpu(enable_counter, NULL, 1);
      } else {
          // disable counter is currently not implemented; reboot resets state
      }
      break;
  }
  
  case CMD_SET_FLUSHING_ENABLED: {
      if(ioctl_param) {
          // enable flushing
          on_each_cpu(enable_flushing, NULL, 1);
      } else {
          // disable flushing is currently not implemented; reboot resets state
      }
      break;
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
    .name = M1_UARCH_MODULE_DEVICE_NAME,
    .fops = &f_ops,
    .mode = S_IRWXUGO,
};


int init_module(void) {
  int r;
  
  /* Register device */
  r = misc_register(&misc_dev);
  if (r != 0) {
    printk(KERN_ALERT "[m1-uarch] Failed registering device with %d\n", r);
    return 1;
  }

  return 0;
}

void cleanup_module(void) {
  misc_deregister(&misc_dev);
  printk(KERN_INFO "[m1-uarch] Removed.\n");
}
