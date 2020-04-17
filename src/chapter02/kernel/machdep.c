#include <stdint.h>
#include "cpu.h"

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

__attribute__((noreturn))
void cstart(void)
{
  if(2) {
    char *s="Hello, world!\r\n";

    init_uart(115200);

    while(*s)
      uart_putc(*s++);
  }

  while(1);
}
