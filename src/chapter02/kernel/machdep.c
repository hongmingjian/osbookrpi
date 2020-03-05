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

void cstart(void)
{
    char *s="Hello, world!\r\n";

    init_uart(9600);

    while(*s)
        sys_putchar(*s++);

    while(1);
}
