@   file:        prep_for_reset.s
@   description: Prepares platform for reset and supplies useful defaults for debugging purposes.
@   target:      Nordic nRF52840

.syntax unified
.thumb
.text
.thumb_func
.global prep_for_reset
.type prep_for_reset, %function

prep_for_reset:
    mov r0, #0
    ldr r1, =#0xe000ed14
    str r0, [r1]
    eor r0, #1
    lsl r2, r0, #5
    strb r2, [r1, #4]
    lsl r3, r2, #8
    orr r2, r3
    strh r2, [r1, #14]
    lsl r2, r0, #6
    strb r2, [r1, #11]
    ldr r1, =#0x20000000
    ldr r2, =#0x00004000
    ldr r0, =#0x55555555
prep_reset_loop:
    subs r2, #4
    str r0, [r1, r2]
    bgt prep_reset_loop
    bx lr
.pool
.size prep_for_reset, . - prep_for_reset
.end
