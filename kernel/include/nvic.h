/** @file   nvic.h
 *  @brief  Holds MMIO addresses for NVIC registers (and likely a NVIC struct eventually) that multiple peripherals need to set
**/

#ifndef _NVIC_H_
#define _NVIC_H_

/** 
 * Memory mapped address of NVIC_ISER0 (each ISER holds a maximum of 32 bits for enabling/disabling interrupt at m+32*n where n is the register number and m is the offset).
 */
#define NVIC_ISER0_ADDR 0xE000E100

#endif