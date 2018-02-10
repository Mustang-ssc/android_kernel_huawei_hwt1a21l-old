/* < DTS2014112007018 liwei 20141120 begin */
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/module.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gfp.h>
#include <asm/io.h>
#include <asm/memory.h>
#include <asm/outercache.h>
#include <linux/spinlock.h>

#include <linux/sched.h>
#include <linux/vmalloc.h>
#include "slt.h"
#include <linux/io.h>
#include <asm/pgtable.h>

extern int fp7_fdct_start(int cpu_id);

static int g_iFdctLoopCount;
static struct device_driver slt_fdct_loop_count_drv =
{
    .name  = "slt_fdct_loop_count",
    .bus   = &platform_bus_type,
    .owner = THIS_MODULE,
};

#define DECLARE_SLT_CPU_GLOBAL_VAR(_N)               \
static int g_iCPU##_N##_PassFail = 0;                \
static int g_iCPU##_N##_Running  = 0;

DECLARE_SLT_CPU_GLOBAL_VAR(0)
DECLARE_SLT_CPU_GLOBAL_VAR(1)
DECLARE_SLT_CPU_GLOBAL_VAR(2)
DECLARE_SLT_CPU_GLOBAL_VAR(3)
DECLARE_SLT_CPU_GLOBAL_VAR(4)
DECLARE_SLT_CPU_GLOBAL_VAR(5)
DECLARE_SLT_CPU_GLOBAL_VAR(6)
DECLARE_SLT_CPU_GLOBAL_VAR(7)


#define DECLARE_SLT_CPU_FDCT_DRV(_N)                     \
static struct device_driver slt_cpu##_N##_fdct_drv =     \
{                                                        \
    .name  = "slt_cpu"#_N"_fdct",                        \
    .bus   = &platform_bus_type,                         \
    .owner = THIS_MODULE,                                \
};

DECLARE_SLT_CPU_FDCT_DRV(0)
DECLARE_SLT_CPU_FDCT_DRV(1)
DECLARE_SLT_CPU_FDCT_DRV(2)
DECLARE_SLT_CPU_FDCT_DRV(3)
DECLARE_SLT_CPU_FDCT_DRV(4)
DECLARE_SLT_CPU_FDCT_DRV(5)
DECLARE_SLT_CPU_FDCT_DRV(6)
DECLARE_SLT_CPU_FDCT_DRV(7)


#define DEFINE_SLT_CPU_FDCT_SHOW(_N)                                                  \
static ssize_t slt_cpu##_N##_fdct_show(struct device_driver *driver, char *buf)       \
{                                                                                     \
    if(g_iCPU##_N##_PassFail == -1)                                                   \
        return snprintf(buf, PAGE_SIZE, "CPU%d fdct - CPU%d is powered off\n",_N,_N); \
    else if (g_iCPU##_N##_Running)                                                    \
        return snprintf(buf, PAGE_SIZE, "CPU%d fdct - Running\n", _N);                \
    else                                                                              \
        return snprintf(buf, PAGE_SIZE, "CPU%d fdct - %s (loop_count = %d)\n", _N,    \
                        g_iCPU##_N##_PassFail != g_iFdctLoopCount ? "FAIL" : "PASS",  \
                        g_iCPU##_N##_PassFail);                                       \
}

DEFINE_SLT_CPU_FDCT_SHOW(0)
DEFINE_SLT_CPU_FDCT_SHOW(1)
DEFINE_SLT_CPU_FDCT_SHOW(2)
DEFINE_SLT_CPU_FDCT_SHOW(3)
DEFINE_SLT_CPU_FDCT_SHOW(4)
DEFINE_SLT_CPU_FDCT_SHOW(5)
DEFINE_SLT_CPU_FDCT_SHOW(6)
DEFINE_SLT_CPU_FDCT_SHOW(7)


#define DEFINE_SLT_CPU_FDCT_STORE(_N)                                                                \
static ssize_t slt_cpu##_N##_fdct_store(struct device_driver *driver, const char *buf, size_t count) \
{                                                                                                    \
    unsigned int i, ret;                                                                             \
    unsigned long mask;                                                                              \
    int retry=0;                                                                                     \
    DEFINE_SPINLOCK(cpu##_N##_lock);                                                                 \
    unsigned long cpu##_N##_flags;                                                                   \
                                                                                                     \
    g_iCPU##_N##_PassFail = 0;                                                                       \
    g_iCPU##_N##_Running = 0;                                                                        \
                                                                                                     \
    mask = (1 << _N); /* processor _N */                                                             \
    while(sched_setaffinity(0, (struct cpumask*) &mask) < 0)                                         \
    {                                                                                                \
        printk("Could not set cpu%d affinity for current process(%d).\n", _N, retry);                \
        g_iCPU##_N##_PassFail = -1;                                                                  \
        retry++;                                                                                     \
        if(retry > 100)                                                                              \
        {                                                                                            \
            return count;                                                                            \
        }                                                                                            \
    }                                                                                                \
                                                                                                     \
    printk("\n>> CPU%d fdct test start (cpu id = %d) <<\n\n", _N, raw_smp_processor_id());           \
    g_iCPU##_N##_Running = 1;                                                                        \
                                                                                                     \
    for (i = 0, g_iCPU##_N##_PassFail = 0; i < g_iFdctLoopCount; i++) {                              \
        spin_lock_irqsave(&cpu##_N##_lock, cpu##_N##_flags);                                         \
        ret = fp7_fdct_start(_N);    /* 1: PASS, 0:Fail, -1: target CPU power off */                 \
        spin_unlock_irqrestore(&cpu##_N##_lock, cpu##_N##_flags);                                    \
        if(ret != -1)                                                                                \
        {                                                                                            \
             g_iCPU##_N##_PassFail += ret;                                                           \
        }                                                                                            \
        else                                                                                         \
        {                                                                                            \
             g_iCPU##_N##_PassFail = -1;                                                             \
             break;                                                                                  \
        }                                                                                            \
    }                                                                                                \
                                                                                                     \
    g_iCPU##_N##_Running = 0;                                                                        \
    if (g_iCPU##_N##_PassFail == g_iFdctLoopCount) {                                                 \
        printk("\n>> CPU%d fdct test pass <<\n\n", _N);                                              \
    }else {                                                                                          \
        printk("\n>> CPU%d fdct test fail (loop count = %d)<<\n\n", _N, g_iCPU##_N##_PassFail);      \
    }                                                                                                \
                                                                                                     \
    return count;                                                                                    \
}

DEFINE_SLT_CPU_FDCT_STORE(0)
DEFINE_SLT_CPU_FDCT_STORE(1)
DEFINE_SLT_CPU_FDCT_STORE(2)
DEFINE_SLT_CPU_FDCT_STORE(3)
DEFINE_SLT_CPU_FDCT_STORE(4)
DEFINE_SLT_CPU_FDCT_STORE(5)
DEFINE_SLT_CPU_FDCT_STORE(6)
DEFINE_SLT_CPU_FDCT_STORE(7)

DRIVER_ATTR(slt_cpu0_fdct, 0644, slt_cpu0_fdct_show, slt_cpu0_fdct_store);
DRIVER_ATTR(slt_cpu1_fdct, 0644, slt_cpu1_fdct_show, slt_cpu1_fdct_store);
DRIVER_ATTR(slt_cpu2_fdct, 0644, slt_cpu2_fdct_show, slt_cpu2_fdct_store);
DRIVER_ATTR(slt_cpu3_fdct, 0644, slt_cpu3_fdct_show, slt_cpu3_fdct_store);
DRIVER_ATTR(slt_cpu4_fdct, 0644, slt_cpu4_fdct_show, slt_cpu4_fdct_store);
DRIVER_ATTR(slt_cpu5_fdct, 0644, slt_cpu5_fdct_show, slt_cpu5_fdct_store);
DRIVER_ATTR(slt_cpu6_fdct, 0644, slt_cpu6_fdct_show, slt_cpu6_fdct_store);
DRIVER_ATTR(slt_cpu7_fdct, 0644, slt_cpu7_fdct_show, slt_cpu7_fdct_store);


static ssize_t slt_fdct_loop_count_show(struct device_driver *driver, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "FDCT Test Loop Count = %d\n", g_iFdctLoopCount);
}

static ssize_t slt_fdct_loop_count_store(struct device_driver *driver, const char *buf, size_t count)
{
    int result;

    if ((result = sscanf(buf, "%d", &g_iFdctLoopCount)) == 1)
    {
        printk("set SLT fdct test loop count = %d successfully\n", g_iFdctLoopCount);
    }
    else
    {
        printk("bad argument!!\n");
        return -EINVAL;
    }

    return count;
}


DRIVER_ATTR(slt_fdct_loop_count, 0644, slt_fdct_loop_count_show, slt_fdct_loop_count_store);

#define DEFINE_SLT_CPU_FDCT_INIT(_N)                                                      \
int __init slt_cpu##_N##_fdct_init(void)                                                  \
{                                                                                         \
    int ret;                                                                              \
                                                                                          \
    ret = driver_register(&slt_cpu##_N##_fdct_drv);                                       \
    if (ret) {                                                                            \
        printk("fail to create SLT CPU%d fdct driver\n",_N);                              \
    }                                                                                     \
                                                                                          \
    ret = driver_create_file(&slt_cpu##_N##_fdct_drv, &driver_attr_slt_cpu##_N##_fdct);   \
    if (ret) {                                                                            \
        printk("fail to create SLT CPU%d fdct sysfs files\n",_N);                         \
    }                                                                                     \
                                                                                          \
    return 0;                                                                             \
}

DEFINE_SLT_CPU_FDCT_INIT(0)
DEFINE_SLT_CPU_FDCT_INIT(1)
DEFINE_SLT_CPU_FDCT_INIT(2)
DEFINE_SLT_CPU_FDCT_INIT(3)
DEFINE_SLT_CPU_FDCT_INIT(4)
DEFINE_SLT_CPU_FDCT_INIT(5)
DEFINE_SLT_CPU_FDCT_INIT(6)
DEFINE_SLT_CPU_FDCT_INIT(7)

int __init slt_fdct_loop_count_init(void)
{
    int ret;

    ret = driver_register(&slt_fdct_loop_count_drv);
    if (ret) {
        printk("fail to create fdct loop count driver\n");
    }

    ret = driver_create_file(&slt_fdct_loop_count_drv, &driver_attr_slt_fdct_loop_count);
    if (ret) {
        printk("fail to create fdct loop count sysfs files\n");
    }

    g_iFdctLoopCount = SLT_LOOP_CNT;

    return 0;
}
arch_initcall(slt_cpu0_fdct_init);
arch_initcall(slt_cpu1_fdct_init);
arch_initcall(slt_cpu2_fdct_init);
arch_initcall(slt_cpu3_fdct_init);
arch_initcall(slt_cpu4_fdct_init);
arch_initcall(slt_cpu5_fdct_init);
arch_initcall(slt_cpu6_fdct_init);
arch_initcall(slt_cpu7_fdct_init);

arch_initcall(slt_fdct_loop_count_init);
/* DTS2014112007018 liwei 20141120 end > */