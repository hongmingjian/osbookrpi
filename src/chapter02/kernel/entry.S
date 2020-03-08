    .extern _cstart        @声明cstart是外部符号

    .text                  @代码段开始
    .globl _entry          @声明_entry是全局符号
_entry:
    ldr pc, vector_reset   @把PC设置为vector_reset处的值,即reset
                           @这条语句相当于跳转到reset处执行
vector_reset:
    .word reset

reset:
    ldr sp, =(0x1000)      @初始化栈

    b _cstart              @跳转到_cstart执行
