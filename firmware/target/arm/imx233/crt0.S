/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"

.section .vectors,"ax",%progbits
.code 32
    /* most handlers are in DRAM which is too far away for a relative jump */
    ldr     pc, =start
    ldr     pc, =undef_instr_handler
    ldr     pc, =software_int_handler
    ldr     pc, =prefetch_abort_handler
    ldr     pc, =data_abort_handler
    ldr     pc, =reserved_handler
    ldr     pc, =irq_handler
    ldr     pc, =fiq_handler

.text
.global start
start:
    /* Save r0 */
    mov     r6, r0
    msr     cpsr_c, #0xd3 /* enter supervisor mode, disable IRQ/FIQ */
    /* Disable MMU, disable caching and buffering;
     * use low exception range address (the core uses high range by default) */
    mrc     p15, 0, r0, c1, c0, 0
    ldr     r1, =0x3005
    bic     r0, r1
    mcr     p15, 0, r0, c1, c0, 0

    /* Zero out IBSS */
    ldr     r2, =_iedata
    ldr     r3, =_iend
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b

    /* Copy the IRAM */
    /* must be done before bss is zeroed */
    ldr     r2, =_iramcopy
    ldr     r3, =_iramstart
    ldr     r4, =_iramend
1:
    cmp     r4, r3
    ldrhi   r5, [r2], #4
    strhi   r5, [r3], #4
    bhi     1b

#ifdef HAVE_INIT_ATTR
    /* copy init data to codec buffer */
    /* must be done before bss is zeroed */
    ldr    r2, =_initcopy
    ldr    r3, =_initstart
    ldr    r4, =_initend
1:
    cmp     r4, r3
    ldrhi   r5, [r2], #4
    strhi   r5, [r3], #4
    bhi     1b

    mov     r2, #0
    mcr     p15, 0, r2, c7, c5, 0   @ Invalidate ICache
#endif

    /* Initialise bss section to zero */
    ldr     r2, =_edata
    ldr     r3, =_end
    mov     r4, #0
1:
    cmp     r3, r2
    strhi   r4, [r2], #4
    bhi     1b

    /* Set up some stack and munge it with 0xdeadbeef */
    ldr     sp, =stackend
    ldr     r2, =stackbegin
    ldr     r3, =0xdeadbeef
1:
    cmp     sp, r2
    strhi   r3, [r2], #4
    bhi     1b
    
    /* Set up stack for IRQ mode */ 
    msr     cpsr_c, #0xd2
    ldr     sp, =irq_stack

    /* Set up stack for FIQ mode */ 
    msr     cpsr_c, #0xd1
    ldr     sp, =fiq_stack

    /* Let abort and undefined modes use IRQ stack */
    msr     cpsr_c, #0xd7
    ldr     sp, =irq_stack
    msr     cpsr_c, #0xdb
    ldr     sp, =irq_stack

    /* Enable MMU */
    bl      memory_init

    /* Switch back to supervisor mode */
    msr     cpsr_c, #0xd3

    /* Jump to main */
    mov     r0, r6
    bl      main
1:
    b       1b

/* All illegal exceptions call into UIE with exception address as first
 * parameter. This is calculated differently depending on which exception
 * we're in. Second parameter is exception number, used for a string lookup
 * in UIE. */
undef_instr_handler:
    sub    r0, lr, #4   @ r0 points to the faulty ARM instruction
#ifdef USE_THUMB
    mrs    r1, spsr
    tst    r1, #(1<<5)  @ T bit set ?
    subne  r0, lr, #2   @ if yes, r0 points to the faulty THUMB instruction
#endif /* USE_THUMB */
    mov    r1, #0
    b      UIE

/* We run supervisor mode most of the time, and should never see a software
 * exception being thrown. Perhaps make it illegal and call UIE? */
software_int_handler:
reserved_handler:
    movs   pc, lr

prefetch_abort_handler:
    sub    r0, lr, #4
    mov    r1, #1
    b      UIE

data_abort_handler:
    sub    r0, lr, #8 
    mov    r1, #2
    b      UIE

/* 256 words of IRQ stack */
    .space 256*4
irq_stack:

/* 256 words of FIQ stack */
    .space 256*4
fiq_stack:

end:
