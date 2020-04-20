#include <string.h>
#include "kernel.h"

static void uart_detach(struct dev *dp)
{
  return;
}

static int uart_ioctl(struct dev *dp, uint32_t cmd,
                      void *arg)
{
  return -1;
}

static int uart_attach(struct dev *dp)
{
  init_uart(115200);
  return 0;
}

static int uart_read(struct dev *dp, uint32_t addr,
                     uint8_t *buf, size_t buf_size)
{
  uint8_t *oldbuf = buf;
  while(buf_size) {
    *buf=uart_getc();
    buf++;
    buf_size--;
  }
  return buf-oldbuf;
}

static int uart_write(struct dev *dp, uint32_t addr,
                      uint8_t *buf, size_t buf_size)
{
  uint8_t *oldbuf = buf;
  while(buf_size) {
    uart_putc(*buf);
    buf++;
    buf_size--;
  }
  return buf-oldbuf;
}

static int uart_poll(struct dev *dp, short events)
{
  int retval = POLLOUT;
  if(events & POLLIN) {
    if(uart_hasc())
      retval |= POLLIN;
  }
  return retval;
}

static struct driver uart_driver = {
  .major  = "uart",
  .attach = uart_attach,
  .detach = uart_detach,
  .read   = uart_read,
  .write  = uart_write,
  .poll   = uart_poll,
  .ioctl  = uart_ioctl
};

struct uart_dev {
  struct dev dev;
} uart_dev = {
  {
  .drv = &uart_driver,
  .minor = 0
  }
};
