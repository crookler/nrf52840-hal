/** @file   arm.h
 *  @brief  Macro definitions, typedefs, and aliases for common ARM functionality.
**/

#ifndef _ARM_H_
#define _ARM_H_

/// Standard Min macro wrapper
#define MIN(_A,_B) (((_A) < (_B)) ? (_A) : (_B))

/// Standard Max macro wrapper
#define MAX(_A,_B) (((_A) > (_B)) ? (_A) : (_B))

/// Null pointer (standard definition)
#define NULL (void *)0

/// Modifier to encourage compiler to inline this function as much as possible
#define intrinsic __attribute__((always_inline)) static inline

/// Execute nop instructions while condition is met
#define BUSY_LOOP(_C) do { asm volatile("nop"); } while(_C)

/// Active waiting running loop with no body for `_N` iterations 
#define COUNTDOWN(_N) for(int _cd_i=_N-1; _cd_i>=0; _cd_i--)

/// MMIO address for the ICSR (used to manually set PendSV interrupt)
#define ICSR *((volatile uint32_t *) 0xe000ed04)

/// MMIO address for the SHCSR (used to read SVC status bit)
#define SHCSR *((volatile uint32_t *) 0xe000ed24)

/// Offset in the SHCSR to actually enable memory management faults
#define MEMFAULT_SHCSR_ENABLE_OFFSET 16

/// MMIO address for the CPACR (used to enable floating point computation)
#define CPACR *((volatile uint32_t *) 0xe000ed88)

/// One byte signed
typedef char int8_t;

/// One byte unsigned
typedef unsigned char uint8_t;

/// Two bytes signed
typedef short int16_t;

/// Two bytes unsigned
typedef unsigned short uint16_t;

/// Four bytes signed (one word)
typedef int int32_t;

/// Four bytes unsigned (one word)
typedef unsigned int uint32_t;

/// Eight bytes signed
typedef long long int64_t;

/// Four bytes unsigned
typedef unsigned long long uint64_t;

/// Direct conversion between C instruction and bkpt Arm assembly instruction at call point
intrinsic void breakpoint() { asm volatile("bkpt"); }

/// Direct conversion between C instruction and wfi Arm assembly instruction at call point
intrinsic void wait_for_interrupt() { asm volatile("wfi"); }

/// Direct conversion between C instruction and cpsie f Arm assembly instruction at call point
intrinsic void enable_interrupts() { asm volatile("cpsie f"); }

/// Direct conversion between C instruction and cpsid f Arm assembly instruction at call point
intrinsic void disable_interrupts() { asm volatile("cpsid f"); }

/// Direct conversion between C instruction and dmb Arm assembly instruction at call point
intrinsic void data_mem_barrier() { asm volatile("dmb"); }

/// Direct conversion between C instruction and dsb Arm assembly instruction at call point
intrinsic void data_sync_barrier() { asm volatile("dsb"); }

/// Direct conversion between C instruction and isb Arm assembly instruction at call point
intrinsic void inst_sync_barrier() { asm volatile("isb"); }

/// Direct conversion between C instruction and wfe (wait for event) Arm assembly instruction at call point
intrinsic void wait_for_event() { asm volatile("wfe"); } 

/// Direct conversion between C instruction and sev (signal event) Arm assembly instruction at call point
intrinsic void send_event() { asm volatile("sev"); }

/// Sets the PendSV bit in the ICSR
intrinsic void set_pendsv() { ICSR |= (1 << 28); }

/// Clears the PendSV bit in the ICSR
intrinsic void clr_pendsv() { ICSR |= (1 << 27); }

/// Get current SVC execution status
intrinsic uint32_t get_svc_status() { return (SHCSR & (1 << 7)); }

/// Set SVC status to provided `status` value (useful for returning to SVC execution or leaving priviledged execution when context switching)
intrinsic void set_svc_status(uint32_t status) {
    if(status)
        SHCSR |= (1 << 7);
    else
        SHCSR &= ~(1 << 7);
}

/// Returns next highest power of 2 above value of n
intrinsic uint32_t ceil_log2(uint32_t n) {
    uint32_t cl2n = 0;
    while(n > (1u << cl2n)) cl2n++;
    return cl2n;
}

/// Enables the floating point unit to handle float types 
intrinsic void enable_fpu() {
    CPACR |= (0xf << 20);
    data_sync_barrier();
    inst_sync_barrier();
}

#endif
