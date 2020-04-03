#ifndef _KERNEL_H
#define _KERNEL_H

#include <sys/types.h>
#include <stdint.h>
#include "board.h"
#include "machdep.h"

#define MMIO_BASE_VA 0xc4000000

void sys_putchar ( int c );

/*中断向量表*/
extern void (*g_intr_vector[])(uint32_t irq, struct context *ctx);

/*让中断控制器打开某个中断*/
void enable_irq(uint32_t irq);

/*让中断控制器关闭某个中断*/
void disable_irq(uint32_t irq);

/*定时器以HZ的频率中断CPU*/
#define HZ   100

/*记录定时器中断的次数*/
extern unsigned volatile g_timer_ticks;

void isr_default(uint32_t irq, struct context *ctx);
void isr_timer(uint32_t irq, struct context *ctx);

void sti(), cli();

int printk(const char *fmt,...);

#define RAM_ZONE_LEN (2 * 8)
extern uint32_t g_ram_zone[RAM_ZONE_LEN];

#define VADDR(pdi, pti) ((uint32_t)(((pdi)<<PGDR_SHIFT)|\
                                    ((pti)<<PAGE_SHIFT)))

#define KERN_MAX_ADDR VADDR(0xFFF, 0xFF)
#define KERN_MIN_ADDR VADDR(0xC00, 0x04)
#define USER_MAX_ADDR VADDR(0xBFC, 0x00)
#define USER_MIN_ADDR VADDR(4, 0)

extern uint32_t *PTD;
extern uint32_t *PT;
#define vtopte(va) (PT+((va)>>PAGE_SHIFT))
#define vtop(va) (((*vtopte(va))&(~PAGE_MASK))|((va)&PAGE_MASK))

#define KERNBASE  VADDR(0xC00, 0)
#define R(x) ((x)-KERNBASE)

#define NR_KERN_PAGETABLE 80

#define IN_USER_VM(va, len) \
       ((uint32_t)(va) >= USER_MIN_ADDR && \
        (uint32_t)(va) <  USER_MAX_ADDR && \
        (uint32_t)(va) + (len) <= USER_MAX_ADDR)

extern uint8_t   end;

void     init_vmspace(uint32_t virtfree);
uint32_t page_alloc(int npages, uint32_t prot, uint32_t user);
uint32_t page_alloc_in_addr(uint32_t va, int npages, uint32_t prot);
int      page_free(uint32_t va, int npages);
uint32_t page_prot(uint32_t va);
#define VM_PROT_NONE   0x00
#define VM_PROT_READ   0x01
#define VM_PROT_WRITE  0x02
#define VM_PROT_EXEC   0x04
#define VM_PROT_RW     (VM_PROT_READ|VM_PROT_WRITE)
#define VM_PROT_ALL    (VM_PROT_RW  |VM_PROT_EXEC)

void     page_map(uint32_t vaddr, uint32_t paddr, uint32_t npages, uint32_t flags);
void     page_unmap(uint32_t vaddr, uint32_t npages);

uint32_t init_frame(uint32_t virtfree);
uint32_t frame_alloc(uint32_t npages);
uint32_t frame_alloc_in_addr(uint32_t pa, uint32_t npages);
void     frame_free(uint32_t paddr, uint32_t npages);

void  init_kmalloc(void *mem, size_t bytes);
void *kmalloc(size_t bytes);
void *krealloc(void *oldptr, size_t size);
void  kfree(void *ptr);
void *kmemalign(size_t align, size_t bytes);

struct tcb {
    uint32_t    kstack;         //内核栈，必须是第一个字段

    int         tid;            //线程ID
    int         state;          //线程状态
#define TASK_STATE_WAITING  -1  //等待
#define TASK_STATE_READY     1  //就绪
#define TASK_STATE_ZOMBIE    2  //僵尸

    int         timeslice;      //时间片
#define TASK_TIMESLICE_DEFAULT 4

    int         code_exit;      //线程的退出代码
    struct wait_queue *wq_exit; //等待该线程退出的线程队列

    struct tcb  *next;

    uint32_t     signature;     //必须是最后一个字段
#define TASK_SIGNATURE 0x20160201
};

#define TASK_KSTACK 0           //=offsetof(struct tcb, kstack)

extern struct tcb *g_task_running;
extern struct tcb *g_task_head;
extern struct tcb *task0;

extern int g_resched;
void schedule();
void switch_to(struct tcb *new);

struct wait_queue {
    struct tcb *tsk;
    struct wait_queue *next;
};
void sleep_on(struct wait_queue **head);
void wake_up(struct wait_queue **head, int n);

void init_task(void);
void syscall(struct context *ctx);
extern void *ret_from_svc;
#endif /*_KERNEL_H*/
