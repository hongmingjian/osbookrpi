  .extern _main, _task_exit

  .globl _start
_start:
  bl _main
  bl _task_exit
