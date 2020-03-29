#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdint.h>
#include "machdep.h"

void sys_putchar ( int c );

/*中断向量表*/
extern void (*g_intr_vector[])(uint32_t irq, struct context *ctx);

/*让中断控制器打开某个中断*/
void enable_irq(uint32_t irq);

/*让中断控制器关闭某个中断*/
void disable_irq(uint32_t irq);

/*定时器以HZ的频率中断CPU*/
#define HZ   100

/*记录定时器中断的次数*/
extern unsigned volatile g_timer_ticks;

void isr_default(uint32_t irq, struct context *ctx);
void isr_timer(uint32_t irq, struct context *ctx);

void sti(), cli();
#endif /*_KERNEL_H*/
