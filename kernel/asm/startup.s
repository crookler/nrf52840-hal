@   file:        startup.s
@   description: vector table and handler definitions
@   target:      Nordic nRF52840

    .syntax unified
    .arch armv7e-m
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

    .section .vector_table
    .global __vector_table
__vector_table:
    .long __kernel_main_stack_base             @ base address of kernel main stack

@
@  Armv7-M system faults and exceptions
@
    .long Reset_Handler
    .long NMI_Handler
    .long HardFault_Handler
    .long MemFault_Handler
    .long BusFault_Handler
    .long UsageFault_Handler
    .long 0
    .long 0
    .long 0
    .long 0
    .long SVC_Handler
    .long DebugMon_Handler
    .long 0
    .long PendSV_Handler
    .long SysTick_Handler

@
@  nRF52840 external interrupts
@
    .long POWER_CLOCK_Handler
    .long RADIO_Handler
    .long UARTE0_Handler
    .long SPIM0_SPIS0_TWIM0_TWIS0_Handler
    .long SPIM1_SPIS1_TWIM1_TWIS1_Handler
    .long NFCT_Handler
    .long GPIOTE_Handler
    .long SAADC_Handler
    .long TIMER0_Handler
    .long TIMER1_Handler
    .long TIMER2_Handler
    .long RTC0_Handler
    .long TEMP_Handler
    .long RNG_Handler
    .long ECB_Handler
    .long CCM_AAR_Handler
    .long WDT_Handler
    .long RTC1_Handler
    .long QDEC_Handler
    .long COMP_LPCOMP_Handler
    .long SWI0_EGU0_Handler
    .long SWI1_EGU1_Handler
    .long SWI2_EGU2_Handler
    .long SWI3_EGU3_Handler
    .long SWI4_EGU4_Handler
    .long SWI5_EGU5_Handler
    .long TIMER3_Handler
    .long TIMER4_Handler
    .long PWM0_Handler
    .long PDM_Handler
    .long ACL_NVMC_Handler
    .long PPI_Handler
    .long MWU_Handler
    .long PWM1_Handler
    .long PWM2_Handler
    .long SPIM2_SPIS2_Handler
    .long RTC2_Handler
    .long I2S_Handler
    .long FPU_Handler
    .long USBD_Handler
    .long UARTE1_Handler
    .long QSPI_Handler
    .long 0
    .long 0
    .long 0
    .long PWM3_Handler
    .long 0
    .long SPIM3_Handler

    .size __vector_table, . - __vector_table


@   function:    Reset_Handler
@   description: system exception handler called when system boots or resets;
@                will do things that we add later, then jump to 'kernel_main'
    .text
    .thumb_func
    .global Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    @ jump to prep_for_reset function (and set link register for returning)
    bl prep_for_reset

    @ populate all addresses between __bss_start and __bss_end with 0's
    @ want the non-kernel versions of these symbols since these are RAM VMA's
    mov r0, #0
    ldr r1, =__bss_start @ get address actually pointed to by label instead of label address
    ldr r2, =__bss_end
Zeros_bss:
    cmp r1, r2 @ check if we reached the end of bss
    it ne
    strne r0, [r1], #4 @ store 0 to address in r1 then increment by 4 (post-index)
    bne Zeros_bss

    @ populate addresses between __data_start and __data_end with initialized values
    @ data will be copied (with the same relative offset) from offset+__data_lma to offset+__data_start
    ldr r3, =__data_lma
    ldr r4, =__data_start
    ldr r5, =__data_end
Initialize_data:
    cmp r4, r5
    itt ne
    ldrne r6, [r3], #4 @ get value from flash (then post-increment)
    strne r6, [r4], #4 @ store data from flash in current RAM pointer (then post-increment)
    bne Initialize_data @ lecture said don't branch in itt block (but seemed to work alright when included)

    @ Go to main execution
    bl kernel_main
    b .
    .pool
    .size Reset_Handler, . - Reset_Handler
    

@   macro:       EXHANDLER
@   description: assembly macro providing weak handler definition that
@                refers to a default handler containing an infinite loop
    .thumb_func
    .global Default_Handler
    .type Default_Handler, %function
Default_Handler:
    bkpt
    b .
    .size Default_Handler, . - Default_Handler

    .macro EXHANDLER handler
    .weak \handler
    .set \handler, Default_Handler
    .endm

@
@  default definitions for system faults and exceptions
@
    EXHANDLER NMI_Handler
    EXHANDLER HardFault_Handler
    EXHANDLER BusFault_Handler
    EXHANDLER UsageFault_Handler
    EXHANDLER DebugMon_Handler
    EXHANDLER SysTick_Handler

@
@  default definitions for external interrupts
@
    EXHANDLER POWER_CLOCK_Handler
    EXHANDLER RADIO_Handler
    EXHANDLER UARTE0_Handler
    EXHANDLER SPIM0_SPIS0_TWIM0_TWIS0_Handler
    EXHANDLER SPIM1_SPIS1_TWIM1_TWIS1_Handler
    EXHANDLER NFCT_Handler
    EXHANDLER GPIOTE_Handler
    EXHANDLER SAADC_Handler
    EXHANDLER TIMER0_Handler
    EXHANDLER TIMER1_Handler
    EXHANDLER TIMER2_Handler
    EXHANDLER RTC0_Handler
    EXHANDLER TEMP_Handler
    EXHANDLER RNG_Handler
    EXHANDLER ECB_Handler
    EXHANDLER CCM_AAR_Handler
    EXHANDLER WDT_Handler
    EXHANDLER RTC1_Handler
    EXHANDLER QDEC_Handler
    EXHANDLER COMP_LPCOMP_Handler
    EXHANDLER SWI0_EGU0_Handler
    EXHANDLER SWI1_EGU1_Handler
    EXHANDLER SWI2_EGU2_Handler
    EXHANDLER SWI3_EGU3_Handler
    EXHANDLER SWI4_EGU4_Handler
    EXHANDLER SWI5_EGU5_Handler
    EXHANDLER TIMER3_Handler
    EXHANDLER TIMER4_Handler
    EXHANDLER PWM0_Handler
    EXHANDLER PDM_Handler
    EXHANDLER ACL_NVMC_Handler
    EXHANDLER PPI_Handler
    EXHANDLER MWU_Handler
    EXHANDLER PWM1_Handler
    EXHANDLER PWM2_Handler
    EXHANDLER SPIM2_SPIS2_Handler
    EXHANDLER RTC2_Handler
    EXHANDLER I2S_Handler
    EXHANDLER FPU_Handler
    EXHANDLER USBD_Handler
    EXHANDLER UARTE1_Handler
    EXHANDLER QSPI_Handler
    EXHANDLER PWM3_Handler
    EXHANDLER SPIM3_Handler

@ Copies the current stack pointer into r0 (easy for assembly) then branches to SVC_C_Handler unconditionally
.thumb_func
.global SVC_Handler
.type SVC_Handler, %function
SVC_Handler:
    mrs r0, psp
    b SVC_C_Handler
.size SVC_Handler, . - SVC_Handler


@ Branch to PendSV_C_Handler (but return to restore values after branch has returned)
.thumb_func
.global PendSV_Handler
.type PendSV_Handler, %function
PendSV_Handler:
    mrs r0, psp @ Cannot push psp directly so load it into r0 first
    push {r0, r4-r11, r14}
    mrs r0, msp @ Load value of MSP into r0 as arguement to PendSV_C_Handler
    bl PendSV_C_Handler @ Do scheduling and return with new MSP in r0 (return value - this is the MSP for the new thread to schedule so pop from it)
    msr msp, r0 @ Put return value in MSP (updated value)
    pop {r0, r4-r11, r14} @ Pop off MSP
    msr psp, r0 @ PSP was in r0 when it was popped
    bx lr @ Branch to value (from restored r14/lr which will include needed context state for how to return)
.size PendSV_Handler, . - PendSV_Handler

@ Branch to MemFault_C_Handler after passing it PSP
.thumb_func
.global MemFault_Handler
.type MemFault_Handler, %function
MemFault_Handler:
    @ Pass PSP to C handler as first arguement
    mrs r0, psp 
    b MemFault_C_Handler
.size MemFault_Handler, . - MemFault_Handler

    .end
    
