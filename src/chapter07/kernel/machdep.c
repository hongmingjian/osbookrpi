#include <stdint.h>
#include <string.h>

#include "arch.h"
#include "cpu.h"
#include "kernel.h"
#include "syscall-nr.h"

void init_uart(uint32_t baud)
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

void uart_putc ( int c )
{
  aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
  while(1) {
    if(aux->mu_lsr&0x20) //Transmitter empty?
      break;
  }
  aux->mu_io = c & 0xff;
}

int uart_hasc()
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
    return aux->mu_lsr&0x1;
}

int uart_getc()
{
    aux_reg_t *aux = (aux_reg_t *)(MMIO_BASE_VA+AUX_REG);
    while(1) {
        if(aux->mu_lsr&0x1)
            break;
    }
    return aux->mu_io & 0xff;
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

  switch(irq) {
  case 0: {
    armtimer_reg_t *pit = (armtimer_reg_t *)
                          (MMIO_BASE_VA+ARMTIMER_REG);
    pit->irqclear = 1;
    break;
    }
  }

  sti();
  g_intr_vector[irq](irq, ctx);
  cli();
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
 * 把CPU从当前线程切换去运行线程new
 */
void switch_to(struct tcb *new)
{
    __asm__ __volatile__ (
            "mrs r12, cpsr\n\t"
            "stmdb sp!, {r4-r12,r14}\n\t"
            "ldr r12, =1f\n\t"
            "stmdb sp!, {r12}\n\t"
            "ldr r12, %0\n\t"
            "str sp, [r12]\n\t"
            "\n\t"
            :
            :"m"(g_task_running)
            );

    g_task_running = new;

    __asm__ __volatile__ (
            "ldr r12, %0\n\t"
            "ldr sp, [r12]\n\t"
            "ldmia sp!, {pc}\n\t"
            "1:\n\t"
            "ldmia sp!, {r4-r12,r14}\n\t"
            "msr cpsr_cxsf, r12\n\t"
            :
            :"m"(g_task_running)
            );
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
   * 物理地址为[0, R(_end)]
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
      "mrc p15,0,r0,c1,c0,0 @SCTLR\n\t"
      "orr r0,r0,%3\n\t"
      "mcr p15,0,r0,c1,c0,0\n\t"
      :
      : "r"(pgdir),
        "r"(1<<5),     /* Disable TTBR1 */
        "r"(1),        /* D0=Client, other=No access */
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
  char *fmt="do_page_fault: 0x%08x(0x%08x)\r\n";

  if((status & (1<<10))/*FS[4]==1*/) {
    printk(fmt, vaddr, status);
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
    printk(fmt, vaddr, status);
    return -1;
  }

  /*检查地址是否合法*/
  prot = page_prot(vaddr);
  if(prot == -1 || prot == VM_PROT_NONE) {
    printk(fmt, vaddr, status);
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
      printk(fmt, vaddr, status);
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
  if(4) {
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

  if(2) {
    init_uart(115200);
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
  }

  if(5) {
    /*
     * 初始化多线程子系统
     */
    init_task();

    /*
     * task0是系统空闲线程，已经由init_task创建。
     * 这里用run_as_task0手工切换到task0运行。
     */
    run_as_task0();
  }

  if(6) {
    /*
     * 初始化SD卡，挂载设备文件系统和FAT文件系统
     */
    extern struct fs     fat_fs, dev_fs;
    extern struct dev    sd_dev, uart_dev, led_dev;

    printk("task #%d: Initializing SD card...", sys_task_getid());
    g_dev_vector[0] = &sd_dev;
    if(g_dev_vector[0]->drv->attach(g_dev_vector[0]))
      printk("Failed\r\n");
    else
      printk("Done\r\n");

    g_dev_vector[1] = &uart_dev;
    g_dev_vector[2] = &led_dev;

    printk("task #%d: Mounting devfs...", sys_task_getid());
    g_fs_vector[0] = &dev_fs;
    if(g_fs_vector[0]->mount(g_fs_vector[0], NULL, -1))
      printk("Failed\r\n");
    else
      printk("Done\r\n");

    printk("task #%d: Mounting FAT...", sys_task_getid());
    g_fs_vector[1] = &fat_fs;
    if(g_fs_vector[1]->mount(g_fs_vector[1], g_dev_vector[0], -1))
      printk("Failed\r\n");
    else
      printk("Done\r\n");
  }

  if(7) {
    /*
     * 加载a.out，并创建用户级线程执行a.out中的main函数
     */
    char *filename="a.out";
    uint32_t entry;

    printk("task #%d: Loading %s...", sys_task_getid(),
           filename);
    entry = load_aout(g_fs_vector[1], filename);

    if(entry) {
      int stksz = 1024*1024;

      printk("Done\r\n");

      printk("task #%d: Creating first user task...",
             sys_task_getid());

      /* 为用户级线程准备栈，大小1MiB */
      page_alloc_in_addr(USER_MAX_ADDR - stksz,
                         stksz/PAGE_SIZE, VM_PROT_RW);

      /* 创建用户线程 */
      if(sys_task_create((void *)USER_MAX_ADDR, (void *)entry,
                         (void *)0x20200416) == NULL)
        printk("Failed\r\n");
    } else
      printk("Failed\r\n");
  }

  while(1)
    ;
}

/**
 * 系统调用分发函数
 */
void syscall(struct context *ctx)
{
  switch(ctx->r4) {
  case SYSCALL_task_exit:
    sys_task_exit(ctx->r0);
    break;
  case SYSCALL_task_create:
    {
      uint32_t user_stack = ctx->r0;
      uint32_t user_entry = ctx->r1;
      uint32_t user_pvoid = ctx->r2;
      struct tcb *tsk;

      if(!IN_USER_VM(user_stack, 0) ||
         !IN_USER_VM(user_entry, 0)) {
        ctx->r0 = -1;
        break;
      }
      tsk = sys_task_create((void *)user_stack,
                            (void (*)(void *))user_entry,
                            (void *)user_pvoid);
      ctx->r0 = (tsk==NULL)?-1:tsk->tid;
    }
    break;
  case SYSCALL_task_getid:
    ctx->r0=sys_task_getid();
    break;
  case SYSCALL_task_yield:
    sys_task_yield();
    break;
  case SYSCALL_task_wait:
    {
      int tid = ctx->r0;
      int *pcode = (int *)(ctx->r1);
      if((pcode != NULL) &&
         !IN_USER_VM(pcode, sizeof(int))) {
        ctx->r0 = -1;
        break;
      }
      ctx->r0 = sys_task_wait(tid, pcode);
    }
    break;
  case SYSCALL_open:
    {
      char *path = (char *)ctx->r0;
      int mode = ctx->r1;
      if(!IN_USER_VM(path, strlen(path)))
        break;
      ctx->r0 = sys_open(path, mode);
      break;
    }
  case SYSCALL_close:
    {
      int fd = ctx->r0;
      ctx->r0 = sys_close(fd);
      break;
    }
  case SYSCALL_read:
    {
      int fd = ctx->r0;
      uint8_t *buffer = (uint8_t *)ctx->r1;
      uint32_t size = ctx->r2;
      if(!IN_USER_VM(buffer, size))
        break;
      ctx->r0 = sys_read(fd, buffer, size);
      break;
    }
  case SYSCALL_write:
    {
      int fd = ctx->r0;
      uint8_t *buffer = (uint8_t *)ctx->r1;
      uint32_t size = ctx->r2;
      if(!IN_USER_VM(buffer, size))
        break;
      ctx->r0 = sys_write(fd, buffer, size);
      break;
    }
  case SYSCALL_lseek:
    {
      int fd = ctx->r0,
          offset = ctx->r1,
          whence = ctx->r2;
      ctx->r0 = sys_seek(fd, offset, whence);
      break;
    }
  case SYSCALL_ioctl:
    {
      int fd = ctx->r0;
      uint32_t cmd = ctx->r1;
      void *arg = (void *)ctx->r2;
      if((arg != NULL) && !IN_USER_VM(arg, 0)) {
        ctx->r0 = -1;
        break;
      }
      ctx->r0 = sys_ioctl(fd, cmd, arg);
      break;
    }
  case SYSCALL_sleep:
    {
      unsigned usec = 1000000 * ctx->r0;
      systimer_reg_t *pst = (systimer_reg_t *)
                            (MMIO_BASE_VA+SYSTIMER_REG);
      uint32_t unow = pst->clo;
      do {
        sys_task_yield();
      } while(pst->clo - unow < usec);
      break;
    }
  default:
      printk("syscall #%d not implemented.\r\n", ctx->r4);
      ctx->r0 = -ctx->r4;
      break;
  }
}
