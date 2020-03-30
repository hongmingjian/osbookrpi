#include <stdint.h>
#include <string.h>

#include "board.h"
#include "kernel.h"

static void init_uart(uint32_t baud)
{
  aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
  gpio_reg_t *gpio = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);

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

void sys_putchar ( int c )
{
  aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
  while(1) {
    if(aux->mu_lsr&0x20/*transmitter empty*/)
      break;
  }
  aux->mu_io = c & 0xff;
}

/**
 * 初始化中断控制器
 */
static void init_pic()
{
  intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_VA+INTR_REG);
  pic->disable_basic_irqs = 0xff;
  pic->disable_irqs_1 = 0xffffffff;
  pic->disable_irqs_2 = 0xffffffff;
}

/**
 * 让中断控制器打开某个中断
 */
void enable_irq(uint32_t irq)
{
  intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_VA+INTR_REG);
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
  intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_VA+INTR_REG);
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
  intr_reg_t *pic = (intr_reg_t *)(MMIO_BASE_VA+INTR_REG);

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

  g_intr_vector[irq](irq, ctx);

  switch(irq) {
  case 0: {
    armtimer_reg_t *pit = (armtimer_reg_t *)
                          (MMIO_BASE_VA+ARMTIMER_REG);
    pit->irqclear = 1;
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
  armtimer_reg_t *pit = (armtimer_reg_t *)
                        (MMIO_BASE_VA+ARMTIMER_REG);
  pit->load = timer_clock/freq;
  pit->reload = pit->load;
  pit->predivider = (SYS_CLOCK_FREQ/timer_clock)-1;
  pit->control = ARMTIMER_CTRL_23BIT |
                 ARMTIMER_CTRL_PRESCALE_1 |
                 ARMTIMER_CTRL_INTR_ENABLE |
                 ARMTIMER_CTRL_ENABLE;
}

/**
 * 初始化分页系统
 */
static uint32_t init_paging(uint32_t physfree)
{
  uint32_t i;
  uint32_t *pgdir, *pte;

  /*
   * 一级页表放在物理地址[0x4000, 0x8000]
   */
  pgdir=(uint32_t *)0x4000;
  memset(pgdir, 0, L1_TABLE_SIZE);

  /*
   * 分配4张二级页表，并填充到一级页表
   * 用于映射二级页表自身所占的逻辑内存范围，
   * 即[0xbfc00000, 0xc0000000]
   */
  uint32_t *ptpte = (uint32_t *)physfree;
  for(i = 0; i < PAGE_SIZE/L2_TABLE_SIZE; i++) {
    pgdir[i+(((uint32_t)PT)>>PGDR_SHIFT)] = (physfree)|L1E_V;
    memset((void *)physfree, 0, L2_TABLE_SIZE);
    physfree+=L2_TABLE_SIZE;
  }
  ptpte[3*L2_ENTRY_COUNT-1] = ((uint32_t)ptpte)|L2E_V|L2E_W|L2E_C;

  /*
   * 分配NR_KERN_PAGETABLE=80张二级页表，并填充到一级页表，
   * 也填充到ptpte
   */
  pte = (uint32_t *)physfree;
  for(i = 0; i < NR_KERN_PAGETABLE; i++) {
    pgdir[i] = pgdir[i+(KERNBASE>>PGDR_SHIFT)] = (physfree)|L1E_V;

    if((i & 3) == 0)
      ptpte[3*L2_ENTRY_COUNT+(i>>2)] = (physfree)|L2E_V|L2E_W|L2E_C;

    memset((void *)physfree, 0, L2_TABLE_SIZE);
    physfree += L2_TABLE_SIZE;
  }

  /*
   * 填充二级页表
   * 映射逻辑地址[0, R(_end)]和[KERNBASE, _end]到
   * 物理地址[0, R(_end)]
   */
  for(i = 0; i < (uint32_t)ptpte; i+=PAGE_SIZE)
    pte[i>>PAGE_SHIFT] = i|L2E_V|L2E_W|L2E_C;

  /*
   * 启动分页
   * ARM1176JZF-S TRM, r0p7, p. 3-57,59,60,63,44 and 6-37
   */
  __asm__ __volatile__ (
      "mcr p15,0,%0,c2,c0,0 @TTBR0\n\t"
      "mcr p15,0,%1,c2,c0,2 @TTBCR\n\t"
      "\n\t"
      "mcr p15,0,%2,c3,c0,0 @DACR\n\t"
      "\n\t"
      "mrc p15,0,r0,c1,c0,0\n\t"
      "orr r0,r0,%3\n\t"
      "mcr p15,0,r0,c1,c0,0\n\t"
      :
      : "r"(pgdir),
        "r"(0),        /* Always use TTBR0 */
        "r"(1<<2),     /* D1=Client, other=No access */
        "r"(1|(1<<23)) /* SCTLR.M=1, SCTLR.XP=1*/
      : "r0"
  );

  return physfree;
}

/**
 * 初始化物理内存
*/
static void init_ram(uint32_t physfree)
{
  g_ram_zone[0] = physfree;
  g_ram_zone[1] = 0x20000000;//512MiB
  g_ram_zone[2] = 0;
  g_ram_zone[3] = 0;
}

__attribute__((noreturn))
void panic(struct context *ctx)
{
  printk("SPSR  : 0x%08x\r\n", ctx->spsr);
  printk("R0    : 0x%08x\r\n", ctx->r0);
  printk("R1    : 0x%08x\r\n", ctx->r1);
  printk("R2    : 0x%08x\r\n", ctx->r2);
  printk("R3    : 0x%08x\r\n", ctx->r3);
  printk("R4    : 0x%08x\r\n", ctx->r4);
  printk("R5    : 0x%08x\r\n", ctx->r5);
  printk("R6    : 0x%08x\r\n", ctx->r6);
  printk("R7    : 0x%08x\r\n", ctx->r7);
  printk("R8    : 0x%08x\r\n", ctx->r8);
  printk("R9    : 0x%08x\r\n", ctx->r9);
  printk("R10   : 0x%08x\r\n", ctx->r10);
  printk("R11   : 0x%08x\r\n", ctx->r11);
  printk("R12   : 0x%08x\r\n", ctx->r12);
  printk("USR_SP: 0x%08x\r\n", ctx->usr_sp);
  printk("USR_LR: 0x%08x\r\n", ctx->usr_lr);
  printk("SVC_SP: 0x%08x\r\n", ctx->svc_sp);
  printk("SVC_LR: 0x%08x\r\n", ctx->svc_lr);
  printk("PC    : 0x%08x\r\n", ctx->pc);

  while(1);
}

int do_page_fault(struct context *ctx, uint32_t vaddr,
                  uint32_t status)
{
  uint32_t prot;

  if((status & (1<<10))/*FS[4]==1*/) {
    printk("do_page_fault: 0x%08x(0x%08x)\r\n", vaddr, status);
    return -1;
  }

  switch((status & 0xf)/*FS[0:3]*/) {
  case 0x5:/*translation fault (section)*/
  /*
   * A section translation fault occurs if:
   * • The TLB fetches a first level translation table descriptor,
   *   and this first level descriptor is invalid. This is the case
   *   when bits[1:0] of this descriptor are b00 or b11.
   *   -- ARM1176JZF-S TRM, r0p7, p. 6-32
   */
  case 0x7:/*translation fault (page)*/
    break;
  default:
    return -1;
  }

  /*检查地址是否合法*/
  prot = page_prot(vaddr);
  if(prot == -1 || prot == VM_PROT_NONE) {
    printk("do_page_fault: 0x%08x(0x%08x)\r\n", vaddr, status);
    return -1;
  }

  {
    uint32_t i, paddr;
    uint32_t flags = L2E_V|L2E_C;

    if(prot & VM_PROT_WRITE)
      flags |= L2E_W;

    /*只要访问用户的地址空间，都代表用户模式访问*/
    if(vaddr < USER_MAX_ADDR)
      flags |= L2E_U;

    /*搜索空闲帧*/
    paddr = frame_alloc(1);
    if(paddr != SIZE_MAX) {
      /*找到空闲帧*/

      /*如果是二级页表引起的缺页，需要填充到一级页表*/
      if(vaddr >= USER_MAX_ADDR && vaddr < KERNBASE) {
        for(i = 0; i < PAGE_SIZE/L2_TABLE_SIZE; i++) {
          PTD[i+(PAGE_TRUNCATE(vaddr)-USER_MAX_ADDR)
                / L2_TABLE_SIZE] = (paddr+i*L2_TABLE_SIZE)|L1E_V;
        }
      }

      *vtopte(vaddr) = paddr|flags;

      invlpg(vaddr);

      memset((void *)(PAGE_TRUNCATE(vaddr)), 0, PAGE_SIZE);

      return 0;
    } else {
      /*物理内存已耗尽*/
      printk("do_page_fault: 0x%08x(0x%08x)\r\n", vaddr, status);
      return -1;
    }
  }

  return -1;
}

void abort_handler(struct context *ctx, uint32_t far, uint32_t fsr)
{
  if(do_page_fault(ctx, far, fsr) == 0)
    return;

  panic(ctx);
}

__attribute__((naked))
static void trampoline()
{
    __asm__ __volatile__("add sp, sp, %0\n\t"
                         "add lr, lr, %0\n\t"
                         "bx  lr\n\t"
                         :
                         : "r" (KERNBASE));
}

__attribute__((noreturn))
void cstart(void)
{
  {
    int i;
    uint32_t virtfree,
             _end = (uint32_t)(&end),
             physfree = PAGE_ROUNDUP(R(_end));
    physfree=init_paging(physfree);

    /*
     * 切换到逻辑地址运行!
     */
    trampoline();

    /*
     * 初始化物理内存管理
     */
    init_ram(physfree);
    virtfree = PAGE_ROUNDUP( _end );
    virtfree = init_frame(virtfree);

    /*
     * 初始化逻辑地址管理，为内核堆预留4MiB的地址空间
     */
    init_vmspace(virtfree+1024*PAGE_SIZE);

    /*
     * 初始化内核堆，大小为4MiB，由kmalloc/kfree管理
     */
    init_kmalloc((uint8_t *)virtfree, 1024*PAGE_SIZE);

    /*
     * 把[MMIO_BASE_VA, MMIO_BASE_VA+16M)保留下来，
     * 并映射到物理地址[MMIO_BASE_PA, MMIO_BASE_PA+16M)
     */
    page_alloc_in_addr(MMIO_BASE_VA, 4096, VM_PROT_RW);
    page_map(MMIO_BASE_VA, MMIO_BASE_PA, 4096, L2E_V|L2E_W);

    /*
     * 把[0xffff0000, 0xffff1000)预留下来，
     * 并映射到[0x8000, 0x9000)
     */
    page_alloc_in_addr(0xffff0000, 1, VM_PROT_RW);
    page_map(0xffff0000, LOADADDR, 1, L2E_V|L2E_W|L2E_C);

    /*
     * 打开hivecs模式
     * ARM1176JZF-S TRM, r0p7, p. 3-44
     */
    __asm__ __volatile__ (
         "mrc p15,0,r0,c1,c0,0\n\t"
         "orr r0, r0, #(1<<13)\n\t" /*SCTLR.V=1*/
         "mcr p15,0,r0,c1,c0,0\n\t"
         :
         :
         : "r0"
    );

    /*
     * 内核已经被重定位到链接地址，取消恒等映射
     */
    for(i = 0; i < NR_KERN_PAGETABLE; i++)
      PTD[i] = 0;

    /* 清空整个TLB
     * ARM1176JZF-S TRM, r0p7, p. 3-86
     */
    __asm__ __volatile__("mcr p15, 0, %[data], c8, c7, 0"
                         :
                         : [data] "r" (0));
  }

  {
    char *s="Hello, world!\r\n";

    init_uart(9600);

    while(*s)
      sys_putchar(*s++);
  }

 {
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

  if(1) {
	int *p = (int *)kmalloc(sizeof(int));
	printk("kmalloc(sizeof(int)) returns 0x%08x\r\n", p);
	*p = 0x5a5a5a5a;
	printk("Its value reads 0x%08x\r\n", *p);

    /*将会触发无法处理的缺页异常*/
    p = (int *)0;
    *p = 0;
  }

  while(1);
}
