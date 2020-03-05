#include "kernel.h"
#include "board.h"

/*中断向量表*/
void (*g_intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

/*默认的中断处理程序*/
void isr_default(uint32_t irq, struct context *ctx)
{
    printk("IRQ=%d\r\n", irq);
}

/*记录系统启动以来，定时器中断的次数*/
unsigned volatile g_timer_ticks = 0;

/**
 * 定时器的中断处理程序
 */
void isr_timer(uint32_t irq, struct context *ctx)
{
    g_timer_ticks++;
    sys_putchar('.');
}

uint32_t *PT  = (uint32_t *)USER_MAX_ADDR, //页表的指针
         *PTD = (uint32_t *)KERN_MIN_ADDR; //页目录的指针
		 
/*可用的物理内存区域*/
uint32_t g_ram_zone[RAM_ZONE_LEN];