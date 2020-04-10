/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2005, 2008, 2013 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#ifndef _MACHDEP_H
#define _MACHDEP_H

#include <stdint.h>

struct context {
    uint32_t spsr;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t usr_sp;
    uint32_t usr_lr;
    uint32_t svc_sp;
    uint32_t svc_lr;
    uint32_t pc;
};

#define STACK_PUSH(sp, value) do {           \
    (sp)-=4;                                 \
    *((uint32_t *)(sp)) = (uint32_t)(value); \
} while(0)

#define INIT_TASK_CONTEXT(ustack, kstack, entry, pv) \
do {                                                 \
    STACK_PUSH(kstack, entry);                       \
    STACK_PUSH(kstack, 0);                           \
    STACK_PUSH(kstack, 0);                           \
    STACK_PUSH(kstack, 0);                           \
    STACK_PUSH(kstack, ustack);                      \
    STACK_PUSH(kstack, 0xCCCCCCCC);                  \
    STACK_PUSH(kstack, 0xBBBBBBBB);                  \
    STACK_PUSH(kstack, 0xAAAAAAAA);                  \
    STACK_PUSH(kstack, 0x99999999);                  \
    STACK_PUSH(kstack, 0x88888888);                  \
    STACK_PUSH(kstack, 0x77777777);                  \
    STACK_PUSH(kstack, 0x66666666);                  \
    STACK_PUSH(kstack, 0x55555555);                  \
    STACK_PUSH(kstack, 0x44444444);                  \
    STACK_PUSH(kstack, 0x33333333);                  \
    STACK_PUSH(kstack, 0x22222222);                  \
    STACK_PUSH(kstack, 0x11111111);                  \
    STACK_PUSH(kstack, pv);                          \
    STACK_PUSH(kstack, (ustack)?0x50:0x53);          \
    STACK_PUSH(kstack, &ret_from_svc);               \
} while(0)

#define run_as_task0() do {         \
    g_task_running = task0;         \
    __asm__ __volatile__ (          \
            "ldr r0, %0\n\t"        \
            "ldr r0, [r0]\n\t"      \
            "ldr r1, =1f\n\t"       \
            "str r1, [r0, #76]\n\t" \
            "mov sp, r0\n\t"        \
            "ldmia sp!, {pc}\n\t"   \
            "1:\n\t"                \
            :                       \
            : "m"(g_task_running)   \
            : "r0", "r1");          \
} while(0)

extern void *ret_from_svc;

#endif /*_MACHDEP_H*/
