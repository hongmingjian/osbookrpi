#define LOADADDR 0x8000

#define PSR_MODE_USR  0x10
#define PSR_MODE_FIQ  0x11
#define PSR_MODE_IRQ  0x12
#define PSR_MODE_SVC  0x13
#define PSR_MODE_ABT  0x17
#define PSR_MODE_UND  0x1b
#define PSR_MODE_SYS  0x1f
#define PSR_MODE_MASK 0x1f

#define PSR_I         0x80

#define CTX_SPSR    0
#define CTX_R0      4
#define CTX_R1      8
#define CTX_R2      12
#define CTX_R3      16
#define CTX_R4      20
#define CTX_R5      24
#define CTX_R6      28
#define CTX_R7      32
#define CTX_R8      36
#define CTX_R9      40
#define CTX_R10     44
#define CTX_R11     48
#define CTX_R12     52
#define CTX_USR_SP  56
#define CTX_USR_LR  60
#define CTX_SVC_SP  64
#define CTX_SVC_LR  68
#define CTX_PC      72
#define CTX_SIZE    76

#define MODSTK_R0   0
#define MODSTK_SPSR 4
#define MODSTK_LR   8
#define MODSTK_SIZE 12

#define PUSHCONTEXT                      \
    sub   lr, lr, #4;                    \
    sub   sp, sp, #MODSTK_SIZE;   /*12*/ \
    str   lr, [sp, #MODSTK_LR];   /* 8*/ \
    mrs   r14, spsr;                     \
    str   r14, [sp, #MODSTK_SPSR];/* 4*/ \
    str   r0, [sp, #MODSTK_R0];   /* 0*/ \
    mov   r0, sp;                        \
    add   sp, sp, #MODSTK_SIZE;   /*12*/ \
    mrs   r14, cpsr;                     \
    bic   r14, r14, #PSR_MODE_MASK;      \
    orr   r14, r14, #PSR_MODE_SVC;       \
    msr   cpsr_cxsf, r14;                \
    sub   sp, sp, #CTX_SIZE;      /*76*/ \
    str   r14, [sp, #CTX_SVC_LR]; /*68*/ \
    ldr   r14, [r0, #MODSTK_LR];  /* 8*/ \
    str   r14, [sp, #CTX_PC];     /*72*/ \
    ldr   r14, [r0, #MODSTK_SPSR];/* 4*/ \
    str   r14, [sp, #CTX_SPSR];   /* 0*/ \
    ldr   r0, [r0, #MODSTK_R0];   /* 0*/ \
    add   r14, sp, #4;                   \
    stmia r14!, {r0-r12};                \
    stmia r14, {r13, r14}^;              \
    str   sp, [sp, #CTX_SVC_SP];  /*64*/

#define PULLCONTEXT                      \
    ldmia sp!, {r0};                     \
    msr   spsr, r0;                      \
    ldmia sp!, {r0-r12};                 \
    ldmia sp, {r13, r14}^;               \
    add   sp, sp, #(3*4);                \
    ldmia sp!, {lr, pc}^

    .extern _cstart               @声明cstart是外部符号
    .extern _edata, _end          @这两个符号在链接器脚本kernel.ld.in中定义
    .extern _irq_handler          @C语言的中断处理函数

    .text                         @代码段开始
    .globl _entry                 @声明_entry是全局符号
_entry:
    ldr pc, vector_reset          @把PC设置为vector_reset处的值,即reset
    ldr pc, vector_undefined      @把PC设置为vector_undefined处的值
    ldr pc, vector_swi            @把PC设置为vector_swi处的值
    ldr pc, vector_prefetch_abort @把PC设置为vector_prefetch_abort处的值
    ldr pc, vector_data_abort     @把PC设置为vector_data_abort处的值
    b   .
    ldr pc, vector_irq            @把PC设置为vector_irq处的值
    b   .

vector_reset:
    .word reset
vector_undefined:
    .word 0
vector_swi:
    .word 0
vector_prefetch_abort:
    .word 0
vector_data_abort:
    .word 0
vector_reserved:
    .word 0
vector_irq:
    .word irq
vector_fiq:
    .word 0

reset:
    mov r0, #LOADADDR @=0x8000
    mov r1, #0x0000
    ldmia r0!, {r2-r9}
    stmia r1!, {r2-r9}
    ldmia r0!, {r2-r9}
    stmia r1!, {r2-r9}

    @ zeroify bss
    ldr r0, =_edata @BSS段起始和
    ldr r1, =_end   @           结束地址
    ldr r2, =0x0
zeroify:
    cmp   r1, r0
    ble   zeroify.1
    stmia r0!, {r2}
    b     zeroify
zeroify.1:

    mrs r0, cpsr
    bic r0, r0, #PSR_MODE_MASK

    orr r1, r0, #PSR_MODE_UND
    msr cpsr_cxsf, r1
    ldr sp, =(0x1000)

    orr r1, r0, #PSR_MODE_ABT
    msr cpsr_cxsf, r1
    ldr sp, =(0x1000-12)

    orr r1, r0, #PSR_MODE_IRQ
    msr cpsr_cxsf, r1
    ldr sp, =(0x1000-24)

    orr r1, r0, #PSR_MODE_SVC
    msr cpsr_cxsf, r1
    ldr sp, =(0x1000-36)

    /* Now enter the C function ... */
    b _cstart                     @跳转到_cstart执行

irq:
    PUSHCONTEXT

    mov r0, sp
    bl  _irq_handler

    PULLCONTEXT

    .globl _sti, _cli
_sti:                           @enable interrupt
    stmfd sp!, {r0, lr}
    mrs   r0, cpsr
    bic   r0, r0, #PSR_I
    msr   cpsr_cxsf, r0
    ldmfd sp!, {r0, pc}

_cli:                           @disable interrupt
    stmfd sp!, {r0, lr}
    mrs   r0, cpsr
    orr   r0, r0, #PSR_I
    msr   cpsr_cxsf, r0
    ldmfd sp!, {r0, pc}
