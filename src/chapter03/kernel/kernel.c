#include "kernel.h"
#include "board.h"

/*中断向量表*/
void (*g_intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

/*默认的中断处理程序*/
void isr_default(uint32_t irq, struct context *ctx)
{
    // do nothing
}

/*记录定时器中断的次数*/
unsigned volatile g_timer_ticks = 0;

/**
 * 定时器的中断处理程序
 */
void isr_timer(uint32_t irq, struct context *ctx)
{
    g_timer_ticks++;
    sys_putchar('.');
}
