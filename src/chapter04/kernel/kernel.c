#include "kernel.h"
#include "cpu.h"

/*中断向量表*/
void (*g_intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

/*默认的中断处理程序*/
void isr_default(uint32_t irq, struct context *ctx)
{
    // do nothing
}

uint32_t *PTD = (uint32_t *)KERN_MIN_ADDR, //一级页表的指针
         *PT  = (uint32_t *)USER_MAX_ADDR; //二级页表的指针

/*可用的物理内存区域*/
uint32_t g_ram_zone[RAM_ZONE_LEN];