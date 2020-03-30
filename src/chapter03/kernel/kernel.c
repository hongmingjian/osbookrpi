#include "kernel.h"
#include "cpu.h"

/*中断向量表*/
void (*g_intr_vector[NR_IRQ])(uint32_t irq, struct context *ctx);

/*默认的中断处理程序*/
void isr_default(uint32_t irq, struct context *ctx)
{
    // do nothing
}
