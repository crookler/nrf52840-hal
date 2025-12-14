/** @file   syscall.h
 *  @brief  Contains stubs and structs useful for handling the SVC handler and letting it support syscalls/additional exceptions.
**/

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "arm.h"

/// Content of the stack frame placed on the stack before the invocation of an svc instruction
typedef struct {
    uint32_t r0; ///< First arguement
    uint32_t r1; ///< Second arguement
    uint32_t r2; ///< Third arguement
    uint32_t r3; ///< Fourth arguement
    uint32_t r12; ///< Previous value of r12
    uint32_t lr; ///< Previous value of lr 
    uint32_t pc; ///< Address of next instruction (execution type stored as immediate value in previous instruction)
    uint32_t xpsr; ///< Previous value of xpsr
} stack_frame_t;

/**
 * Offers support for multiple software-pended exceptions/syscalls through use of single SVC_Handler.
 * Will receive pointer to process stack as arguement (loaded into r0 by SVC_Handler assembly func which calls this C-level handler).  
 */ 
void SVC_C_Handler(void *psp);

/**
 * sbrk system call implementation supporting NEWLIB.
 */
void *syscall_sbrk(int incr);

/**
 * write system call implementation supporting NEWLIB.
 */
int syscall_write(int file, char *ptr, int len);

/**
 * read system call implementation supporting NEWLIB.
 */
int syscall_read(int file, char *ptr, int len);

/**
 * exit system call implementation supporting NEWLIB.
 */
void syscall_exit(int status);

#endif