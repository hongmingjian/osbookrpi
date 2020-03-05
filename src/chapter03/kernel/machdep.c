#include <stdint.h>

#include "board.h"
#include "kernel.h"

static void init_uart(uint32_t baud)
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_PA+AUX_REG);
    gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_PA+GPIO_REG);

    aux->enables = 1;
    aux->mu_ier = 0;
    aux->mu_cntl = 0;
    aux->mu_lcr = 3;
    aux->mu_mcr = 0;
    aux->mu_baud = (SYS_CLOCK_FREQ/(8*baud))-1;

    uint32_t ra=gpio->gpfsel1;
    ra&=~(7<<12); //gpio14
    ra|=2<<12;    //alt5
    ra&=~(7<<15); //gpio15
    ra|=2<<15;    //alt5
    gpio->gpfsel1 = ra;

    gpio->gppud = 0;
    gpio->gppudclk0 = (1<<14)|(1<<15);
    gpio->gppudclk0 = 0;

    aux->mu_cntl = 3;
}

void sys_putchar ( int c )
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_PA+AUX_REG);
    while(1) {
        if(aux->mu_lsr&0x20)
            break;
    }
    aux->mu_io = c;
}

/**
 * 初始化中断控制器
 */
static void init_pic()
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_PA+INTR_REG);
    pic->Disable_basic_IRQs = 0xff;
    pic->Disable_IRQs_1 = 0xffffffff;
    pic->Disable_IRQs_2 = 0xffffffff;
}

/**
 * 让中断控制器打开某个中断
 */
void enable_irq(uint32_t irq)
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_PA+INTR_REG);
    if(irq  < 8) {
        pic->Enable_basic_IRQs = 1 << irq;
    } else if(irq < 40) {
        pic->Enable_IRQs_1 = 1<<(irq - 8);
    } else if(irq < 72) {
        pic->Enable_IRQs_2 = 1<<(irq - 40);
    }
}

/**
 * 让中断控制器关闭某个中断
 */
void disable_irq(uint32_t irq)
{
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_PA+INTR_REG);
    if(irq  < 8) {
        pic->Disable_basic_IRQs = 1 << irq;
    } else if(irq < 40) {
        pic->Disable_IRQs_1 = 1<<(irq - 8);
    } else if(irq < 72) {
        pic->Disable_IRQs_2 = 1<<(irq - 40);
    }
}

void irq_handler(struct context *ctx)
{

    int irq;
    intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_PA+INTR_REG);

    for(irq = 0 ; irq < 8; irq++)
        if(pic->IRQ_basic_pending & (1<<irq))
            break;

    if(irq == 8){
        for( ; irq < 40; irq++)
            if(pic->IRQ_pending_1 & (1<<(irq-8)))
                break;

        if(irq == 40) {
            for( ; irq < 72; irq++)
                if(pic->IRQ_pending_1 & (1<<(irq-40)))
                    break;

            if(irq == 72)
                return;
        }
    }

    g_intr_vector[irq](irq, ctx);

    switch(irq) {
    case 0: {
        armtimer_reg_t *pit = (armtimer_reg_t *)(MMIO_BASE_PA+ARMTIMER_REG);
        pit->IRQClear = 1;
        break;
    }
    }
}

/**
 * 初始化定时器
 */
static void init_pit(uint32_t freq)
{
    uint32_t timer_clock = 1000000;
    armtimer_reg_t *pit = (armtimer_reg_t *)(MMIO_BASE_PA+ARMTIMER_REG);
    pit->Load = timer_clock/freq;
    pit->Reload = pit->Load;
    pit->PreDivider = (SYS_CLOCK_FREQ/timer_clock)-1;
    pit->Control = ARMTIMER_CTRL_23BIT |
                ARMTIMER_CTRL_PRESCALE_1 |
                ARMTIMER_CTRL_INTR_ENABLE |
                ARMTIMER_CTRL_ENABLE;
}

void cstart(void)
{
	{
		char *s="Hello, world!\r\n";

		init_uart(9600);

		while(*s)
			sys_putchar(*s++);
	}

    {
		int i;

		init_pic();
		init_pit(HZ);

        /*安装默认的中断处理程序*/
        for(i = 0; i < NR_IRQ; i++)
            g_intr_vector[i]=isr_default;

        /*安装定时器的中断处理程序*/
        g_intr_vector[IRQ_TIMER] = isr_timer;
        enable_irq(IRQ_TIMER);

		sti();
    }

    while(1);
}
