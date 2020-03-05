#include "kernel.h"
#include "board.h"

/*�ж�������*/
void (*g_intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

/*Ĭ�ϵ��жϴ������*/
void isr_default(uint32_t irq, struct context *ctx)
{
    printk("IRQ=%d\r\n", irq);
}

/*��¼ϵͳ������������ʱ���жϵĴ���*/
unsigned volatile g_timer_ticks = 0;

/**
 * ��ʱ�����жϴ������
 */
void isr_timer(uint32_t irq, struct context *ctx)
{
    g_timer_ticks++;
    sys_putchar('.');
}

uint32_t *PT  = (uint32_t *)USER_MAX_ADDR, //ҳ���ָ��
         *PTD = (uint32_t *)KERN_MIN_ADDR; //ҳĿ¼��ָ��
		 
/*���õ������ڴ�����*/
uint32_t g_ram_zone[RAM_ZONE_LEN];