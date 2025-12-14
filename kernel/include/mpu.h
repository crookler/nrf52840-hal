/** @file   mpu.h
 *  @brief  MMIO addresses and common enums/structs for working with the memory protection unit.
**/

#ifndef _MPU_H_
#define _MPU_H_

#include "arm.h"

/** 
 * Specifies how to isolate memory for a running program.
 */
typedef enum {
    KERNEL_PROTECT, ///< Only the kernel should have its memory protected
    THREAD_PROTECT ///< Kernel and threads should be isolated in memory from each other
} mpu_mode;

/** @brief mpu control register */
#define MPU_CTRL    *((volatile uint32_t *)0xe000ed94)
/** @brief mpu region number register */
#define MPU_RNR     *((volatile uint32_t *)0xe000ed98)
/** @brief mpu region base address register */
#define MPU_RBAR    *((volatile uint32_t *)0xe000ed9c)
/** @brief mpu region attribute and size register */
#define MPU_RASR    *((volatile uint32_t *)0xe000eda0)

/** @brief configurable fault status register */
#define CFSR        *((volatile uint32_t *)0xe000ed28)
/** @brief mem fault address register */
#define MMFAR       *((volatile uint32_t *)0xe000ed34)

/** @brief MPU CTRL register (~ARM p.637) */
//@{
#define MPU_CTRL_PRIVDEFENA_POS (2)         /*<! bit offset of flag to enable default mem map as background region */
#define MPU_CTRL_HFNMIENA_POS   (1)         /*<! bit offset of flag to enable MPU for fault handlers as unprivileged code */
#define MPU_CTRL_ENABLE_POS     (0)         /*<! bit offset of flag to enable MPU for unprivileged code */
//@}

/** @brief MPU RNR register (~ARM p.638) */
//@{
#define MPU_RNR_REGION_POS  (0)             /*<! bit offset of region number field ref'd by RBAR and RASR */
#define MPU_RNR_REGION_MAX  (7)             /*<! max value of region number field */
//@}

/** @brief MPU RBAR register (~ARM p.639) */
//@{
#define MPU_RBAR_ADDR_MASK  (0xffffffe0)    /*<! base address (32B aligned) to apply to selected region */
//@}

/** @brief MPU RASR register (~ARM p.640) */
//@{
#define MPU_RASR_XN_POS     (28)            /*<! bit offset of execute-never bit in RASR */
#define MPU_RASR_AP_POS     (24)            /*<! bit offset of access and privilege field in RASR */
#define MPU_RASR_AP_MAX     (7)             /*<! max value of access and privilege field (~ARM, Table B3-15) */
#define MPU_RASR_AP_RO      (2)             /*<! unprivileged read-only --> AP = 0b010 */
#define MPU_RASR_AP_RW      (3)             /*<! unprivileged read/write --> AP = 0b011 */
#define MPU_RASR_SIZE_POS   (1)             /*<! bit offset of region size field in RASR */
#define MPU_RASR_SIZE_MIN   (4)             /*<! min value in region size field (size is 2^{1+value}) */
#define MPU_RASR_SIZE_MAX   (31)            /*<! max value in region size field (size is 2^{1+value}) */
#define MPU_RASR_ENABLE_POS (0)             /*<! bit offset of region enable bit */
//@}

/** @brief CFSR / MMFSR register (~ARM p.609) */
//@{
#define CFSR_IACCVIOL_POS   (0)             /*<! bit offset of instruction access violation flag */
#define CFSR_DACCVIOL_POS   (1)             /*<! bit offset of data access violation flag */
#define CFSR_MUNSTKERR_POS  (3)             /*<! bit offset of unstacking violation flag */
#define CFSR_MSTKERR_POS    (4)             /*<! bit offset of stacking violation flag */
#define CFSR_MMFARVALID_POS (7)             /*<! bit offset of MMFAR validity flag */
//@}

/**
 * Enables the memory protection unit unconditionally (always want to protect kernel).
 */
void mpu_enable();

/**
 * Disables the memory protection unit
 */
void mpu_disable(); 

/**
 * Enables a memory protection region for a user stack (indicated by `base_addr` with allocated `size`).
 */
void mpu_thread_region_enable(void *base_addr, uint32_t size);

/**
 * Disables the enabled user stack protection region.
 */
void mpu_thread_region_disable();

/**
 * Enables a memory protection region for a kernel stack (specified as starting at `base_addr` with allocated `size`).
 */
void mpu_kernel_region_enable(void *base_addr, uint32_t size);

/**
 * Disables the enabled kernel stack protection region.
 */
void mpu_kernel_region_disable();

#endif