#ifndef _KERNEL_H
#define _KERNEL_H

#include <stdint.h>
#include "machdep.h"

void sys_putchar ( int c );

/*�ж�������*/
extern void (*g_intr_vector[])(uint32_t irq, struct context *ctx);

/*���жϿ�������ĳ���ж�*/
void enable_irq(uint32_t irq);

/*���жϿ������ر�ĳ���ж�*/
void disable_irq(uint32_t irq);

/*��ʱ����HZ��Ƶ���ж�CPU*/
#define HZ   100

/*��¼ϵͳ������������ʱ���жϵĴ���*/
extern unsigned volatile g_timer_ticks;

void isr_default(uint32_t irq, struct context *ctx);
void isr_timer(uint32_t irq, struct context *ctx);

void sti(), cli();
#endif /*_KERNEL_H*/
