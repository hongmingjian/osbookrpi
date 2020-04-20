#include <string.h>
#include "cpu.h"
#include "kernel.h"

static void _detach(struct dev *dp)
{
	return;
}

static int _readwrite(struct dev *dp, uint32_t addr,
                      uint8_t *buf, size_t buf_size)
{
	return -1;
}

static int led_attach(struct dev *dp);
static int led_ioctl(struct dev *dp, uint32_t cmd,
                     void *arg);

static struct driver led_driver = {
	.major  = "led",
	.attach = led_attach,
	.detach = _detach,
	.read   = _readwrite,
	.write  = _readwrite,
	.ioctl  = led_ioctl
};

struct led_dev {
	struct dev dev;
	int gpio;
	int active_low;
} led_dev = {
	{
	.drv   = &led_driver,
	.minor = 0
	},
	16,
	1
};

static int led_attach(struct dev *dp)
{
	struct led_dev *ldp = (struct led_dev *)dp;
	gpio_reg_t *grt = (gpio_reg_t *)
	                  (MMIO_BASE_VA+GPIO_REG);

	uint32_t *p = (uint32_t *)(((uint32_t)
	                            (&grt->gpfsel0))+
                               (ldp->gpio/10)*4);

	int bit=(ldp->gpio%10)*3;

	uint32_t old = *p;
	old &= ~(7<<bit);
	old |= (1<<bit); //as output
	*p = old;
	return 0;
}

static int led_ioctl(struct dev *dp, uint32_t cmd,
                     void *arg)
{
	struct led_dev *ldp = (struct led_dev *)dp;
	gpio_reg_t *grt = (gpio_reg_t *)
	                  (MMIO_BASE_VA+GPIO_REG);
	uint32_t *p;

	if(cmd)
		cmd = 1;

	if(ldp->active_low)
		cmd ^= 1;

	p = (uint32_t *)((uint32_t)(cmd?(&grt->gpset0):
	                                (&grt->gpclr0))+
                         (ldp->gpio/32)*4);
	*p = 1<<(ldp->gpio&0x1f);
	return 0;
}
