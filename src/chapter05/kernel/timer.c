#include <stddef.h>
#include "kernel.h"

/*记录定时器中断的次数*/
unsigned volatile g_timer_ticks = 0;

/**
 * 定时器的中断处理程序
 */
void isr_timer(uint32_t irq, struct context *ctx)
{
    g_timer_ticks++;
    sys_putchar('.');
    
    if(g_task_running != NULL) {
        //如果是task0在运行，则强制调度
        if(g_task_running->tid == 0) {
            g_resched = 1;
        } else {
            //否则，把当前线程的时间片减一
            --g_task_running->timeslice;

            //如果当前线程用完了时间片，也要强制调度
            if(g_task_running->timeslice <= 0) {
                g_resched = 1;
                g_task_running->timeslice = TASK_TIMESLICE_DEFAULT;
            }
        }
    }    
}