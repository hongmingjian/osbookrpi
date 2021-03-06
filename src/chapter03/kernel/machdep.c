#include <stdint.h>

#include "arch.h"
#include "cpu.h"
#include "kernel.h"

void init_uart(uint32_t baud)
{
  aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_PA+AUX_REG);
  gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_PA+GPIO_REG);

  aux->enables = 1; // enable mini UART
  aux->mu_ier = 0;  // no interrupts
  aux->mu_cntl = 0; // disable transmitter & receiver
  aux->mu_lcr = 3;  // 8bit
  aux->mu_mcr = 0;  // RTS line high
  aux->mu_baud = (SYS_CLOCK_FREQ/(8*baud))-1;

  uint32_t ra=gpio->gpfsel1;
  ra&=~(7<<12); // gpio14
  ra|=2<<12;    //  alt5
  ra&=~(7<<15); // gpio15
  ra|=2<<15;    //  alt5
  gpio->gpfsel1 = ra;

  gpio->gppud = 0; // disable pull-up/down

  // wait 150 cycles
  {ra = 150; while(ra--) __asm__ __volatile__("nop");}

  gpio->gppudclk0 = (1<<14)|(1<<15); // assert clock on 14 & 15

  // wait 150 cycles
  {ra = 150; while(ra--) __asm__ __volatile__("nop");}

  gpio->gppudclk0 = 0; // off

  aux->mu_cntl = 3; // enable transmitter & receiver
}

void uart_putc ( int c )
{
  aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_PA+AUX_REG);
  while(1) {
    if(aux->mu_lsr&0x20) //Transmitter empty?
      break;
  }
  aux->mu_io = c & 0xff;
}

/**
 * 初始化中断控制器
 */
static void init_pic()
{
  intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_PA+INTR_REG);
  pic->disable_basic_irqs = 0xff;
  pic->disable_irqs_1 = 0xffffffff;
  pic->disable_irqs_2 = 0xffffffff;
}

/**
 * 让中断控制器打开某个中断
 */
void enable_irq(uint32_t irq)
{
  intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_PA+INTR_REG);
  if(irq  < 8) {
    pic->enable_basic_irqs = 1 << irq;
  } else if(irq < 40) {
    pic->enable_irqs_1 = 1<<(irq - 8);
  } else if(irq < NR_IRQ) {/*=72*/
    pic->enable_irqs_2 = 1<<(irq - 40);
  }
}

/**
 * 让中断控制器关闭某个中断
 */
void disable_irq(uint32_t irq)
{
  intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_PA+INTR_REG);
  if(irq  < 8) {
    pic->disable_basic_irqs = 1 << irq;
  } else if(irq < 40) {
    pic->disable_irqs_1 = 1<<(irq - 8);
  } else if(irq < NR_IRQ) {/*=72*/
    pic->disable_irqs_2 = 1<<(irq - 40);
  }
}

void irq_handler(struct context *ctx)
{
  int irq;
  intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_PA+INTR_REG);

  for(irq = 0 ; irq < 8; irq++)
    if(pic->irq_basic_pending & (1<<irq))
      break;

  if(irq == 8){
    for( ; irq < 40; irq++)
      if(pic->irq_pending_1 & (1<<(irq-8)))
        break;

    if(irq == 40) {
      for( ; irq < NR_IRQ; irq++)
        if(pic->irq_pending_1 & (1<<(irq-40)))
          break;

      if(irq == NR_IRQ)/*=72*/
        return;
    }
  }
  
  switch(irq) {
  case 0: {
    armtimer_reg_t *pit = (armtimer_reg_t *)
                          (MMIO_BASE_PA+ARMTIMER_REG);
    pit->irqclear = 1;
    break;
    }
  }
  
  g_intr_vector[irq](irq, ctx);
}

/**
 * 初始化定时器
 */
static void init_pit(uint32_t freq)
{
  uint32_t timer_clock = 1000000;
  armtimer_reg_t *pit = (armtimer_reg_t *)
                        (MMIO_BASE_PA+ARMTIMER_REG);
  pit->load = timer_clock/freq;
  pit->reload = pit->load;
  pit->predivider = (SYS_CLOCK_FREQ/timer_clock)-1;
  pit->control = ARMTIMER_CTRL_23BIT |
                 ARMTIMER_CTRL_PRESCALE_1 |
                 ARMTIMER_CTRL_INTR_ENABLE |
                 ARMTIMER_CTRL_ENABLE;
}

__attribute__((noreturn))
void cstart(void)
{
  if(2) {
    char *s="Hello, world!\r\n";

    init_uart(115200);

    while(*s)
      uart_putc(*s++);
  }

  if(3) {
    int i;

    /*初始化中断控制器和定时器*/
    init_pic();
    init_pit(HZ); // HZ=100

    /*安装默认的中断处理程序*/
    for(i = 0; i < NR_IRQ; i++)
      g_intr_vector[i]=isr_default;

    /*安装定时器的中断处理程序*/
    g_intr_vector[IRQ_TIMER] = isr_timer;
    enable_irq(IRQ_TIMER);

    /*CPSR.I=0*/
    sti();
  }

  while(1);
}
