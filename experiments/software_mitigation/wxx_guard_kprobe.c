#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/sched/signal.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/version.h>

MODULE_DESCRIPTION("W^X guard via kprobes (log/kill) for mmap/mprotect/pkey_mprotect/shmat");
MODULE_AUTHOR("Michael Schwarz");
MODULE_LICENSE("GPL");

// ---- Tunables -------------------------------------------------------------
static int action = 0; // 0=log, 1=SIGKILL
module_param(action, int, 0644);
MODULE_PARM_DESC(action, "0=log, 1=send SIGKILL");

static int allow_pid = -1; // PID allowlist
module_param(allow_pid, int, 0644);
MODULE_PARM_DESC(allow_pid, "PID to allow (skip enforcement). -1 disables");

#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef PROT_EXEC
#define PROT_EXEC  0x4
#endif
#ifndef SHM_EXEC
#define SHM_EXEC  0100000
#endif

static inline bool pid_allowed(void)
{
    return (allow_pid > 0) && (task_pid_nr(current) == allow_pid);
}

static void punish_task(const char *why)
{
    if (action == 0)
        return;

    pr_warn_ratelimited("wxx_kp: BLOCK %s pid=%d comm=%s\n", why, task_pid_nr(current), current->comm);
    if (action == 1) {
        send_sig(SIGKILL, current, 0);
    }
}

#define log_event printk

static inline bool is_wx(unsigned long prot)
{
    return (prot & (PROT_WRITE | PROT_EXEC)) == (PROT_WRITE | PROT_EXEC);
}

static int kp_mmap_pre(struct kprobe *kp, struct pt_regs *regs)
{
#ifdef CONFIG_X86_64
    unsigned long prot = regs->cx;
    if (!pid_allowed() && is_wx(prot)) {
        log_event("wxx_kp: mmap W+X pid=%d comm=%s len=%lu prot=0x%lx\n",
                  task_pid_nr(current), current->comm, regs->dx, prot);
        punish_task("mmap W+X");
    }
#else
//	log_event("wxx_kp: %zx %zx %zx %zx %zx %zx %zx %zx\n", regs->regs[0], regs->regs[1], regs->regs[2], regs->regs[3], regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
    unsigned long prot = regs->regs[3];
    if (!pid_allowed() && is_wx(prot)) {
        log_event("wxx_kp: mmap W+X pid=%d comm=%s len=%lu prot=0x%lx\n",
                  task_pid_nr(current), current->comm, regs->regs[2], prot);
        punish_task("mmap W+X");
    }

#endif
    return 0;
}

static int kp_mprotect_pre(struct kprobe *kp, struct pt_regs *regs)
{
#ifdef CONFIG_X86_64
    unsigned long prot = regs->dx;
    if (!pid_allowed() && is_wx(prot)) {
        log_event("wxx_kp: mprotect W+X pid=%d comm=%s addr=0x%lx len=%lu prot=0x%lx\n",
                  task_pid_nr(current), current->comm, regs->di, regs->si, prot);
        punish_task("mprotect W+X");
    }
#endif
    return 0;
}


static int kp_shmat_pre(struct kprobe *kp, struct pt_regs *regs)
{
#ifdef CONFIG_X86_64
    unsigned long shmflg = regs->dx;
    if (!pid_allowed() && (shmflg & SHM_EXEC)) {
        log_event("wxx_kp: shmat with SHM_EXEC pid=%d comm=%s shmid=%lu flags=0%lo\n",
                  task_pid_nr(current), current->comm, regs->di, shmflg);
        punish_task("shmat SHM_EXEC");
    }
#endif
    return 0;
}

// ---- Registration ---------------------------------------------------------
static struct kprobe kp_mmap   = { .symbol_name = "do_mmap",          .pre_handler = kp_mmap_pre };
static struct kprobe kp_mprot  = { .symbol_name = "__arm64_sys_mprotect" /*do_mprotect_pkey"*/, .pre_handler = kp_mprotect_pre };
static struct kprobe kp_shmat  = { .symbol_name = "do_shmat",         .pre_handler = kp_shmat_pre };

static int __init wxx_kp_init(void)
{
    int ret;

#ifndef CONFIG_KPROBES
    pr_err("wxx_kp: CONFIG_KPROBES is required\n");
    return -EINVAL;
#endif

    if ((ret = register_kprobe(&kp_mmap))   ) goto fail0;
    if ((ret = register_kprobe(&kp_mprot))  ) goto fail1;
    if ((ret = register_kprobe(&kp_shmat))  ) goto fail2;

    pr_info("wxx_kp: loaded (action=%d, allow_pid=%d)\n", action, allow_pid);
    return 0;

fail2: unregister_kprobe(&kp_mprot);
fail1: unregister_kprobe(&kp_mmap);
fail0:
    pr_err("wxx_kp: failed to register kprobes: %d\n", ret);
    return ret;
}

static void __exit wxx_kp_exit(void)
{
    unregister_kprobe(&kp_shmat);
    unregister_kprobe(&kp_mprot);
    unregister_kprobe(&kp_mmap);
    pr_info("wxx_kp: unloaded\n");
}

module_init(wxx_kp_init);
module_exit(wxx_kp_exit);

