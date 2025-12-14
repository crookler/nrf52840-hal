/* file:        linker_template.lds
 * description: linker script template specific to nRF52840 with placeholder 
 *              values populated by the Makefile on each build
 */

/* space allocated for maximum 2kB stacks, 8kB heap */
__stack_size = 2K;
__heap_size = 8K;

/* nRF52840 has 1MB flash at offset 0 and 256kB RAM at offset 0x20000000 */
MEMORY {
    flash   (rx)  : ORIGIN = 0, LENGTH = 1M
    ram     (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

SECTIONS {

    /* kernel text at address 0, starting with vector table */
    .text : {
        __vector_table_start = .;
        KEEP(*(.vector_table))
        __vector_table_end = .;

        __kernel_text_start = .;
        <K_OBJ_DIR>/*.o (.text*)               /* template for Makefile */
        <K_LIB>:*.o (.text*)                   /* template for Makefile */
        __kernel_text_end = .;
    } > flash
    
    /* user text -- 16k aligned, starting with syscall stubs */
    .user_text : ALIGN(16K) {
        __svc_stub_start = .;
        KEEP(*(.svc_stub))
        __svc_stub_end = .;

        __user_text_start = .;
        <U_OBJ_DIR>/*.o (.text*)               /* template for Makefile */
        *(.text*)
        __user_text_end = .;
    } > flash

    /* read-only data -- 16k aligned */
    .rodata : ALIGN(16K) {
        __kernel_rodata_start = .;
        <K_OBJ_DIR>/*.o (.rodata*)             /* template for Makefile */
        <K_LIB>:*.o (.rodata*)                 /* template for Makefile */
        __kernel_rodata_end = .;
    } > flash

    /* user read-only data -- 2k aligned */
    .user_rodata : ALIGN(2K) {
        __user_rodata_start = .;
        <U_OBJ_DIR>/*.o (.rodata*)             /* template for Makefile */
        *(.rodata*)
        __user_rodata_end = .;
    } > flash

    /* this section is used for exception stacking and backtrace capabilities */
    .ARM.exidx : ALIGN(8) {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > flash

    . = ALIGN(2K);
    __data_lma = .;

    /* data section includes in-memory RTT data structure followed by all kernel data */
    .data : AT (__data_lma) {
        __data_start = .;
        __rtt_start = .;
        . += 168;
        __rtt_end = .;
        __kernel_data_start = .;
        <K_OBJ_DIR>/*.o (.data*)               /* template for Makefile */
        <K_LIB>:*.o (.data*)                   /* template for Makefile */
        __kernel_data_end = .;

        /* user data -- 1k aligned */
        . = ALIGN(1K);
        __user_data_start = .;
        <U_OBJ_DIR>/*.o (.data*)                 /* template for Makefile */
        *(.data*)
        __user_data_end = .;

        . = ALIGN(4);
        __data_end = .;
    } > ram

    /* bss section includes all uninitialized variables used in the kernel */
    .bss : {
        . = ALIGN(1K);
        __bss_start = .;
        __kernel_bss_start = .;
        <K_OBJ_DIR>/*.o (.bss*)                /* template for Makefile */
        <K_OBJ_DIR>/*.o (COMMON*)              /* template for Makefile */
        <K_LIB>:*.o (.bss*)                    /* template for Makefile */
        <K_LIB>:*.o (COMMON*)                  /* template for Makefile */
        __kernel_bss_end = .;

        /* user bss -- 1k aligned */
        . = ALIGN(1K);
        __user_bss_start = .;
        <U_OBJ_DIR>/*.o (.bss*)                  /* template for Makefile */
        <U_OBJ_DIR>/*.o (COMMON*)                /* template for Makefile */
        *(.bss*) *(COMMON*)
        __user_bss_end = .;

        . = ALIGN(4);
        __bss_end = .;
    } > ram

    /* misc section used to create boundary labels for stacks and heap */
    .misc : {
        . = ALIGN(8K);
        
        /* 8kB heap for _sbrk */
        __heap_base = .;
        . += __heap_size;
        __heap_limit = .;
        
        /* 2kB user stack */
        __user_process_stack_limit = .;
        . += __stack_size;
        __user_process_stack_base = .;
        
        /* 2kB kernel stack */
        __kernel_main_stack_limit = .;
        . += __stack_size;
        __kernel_main_stack_base = .;
        
        . = ALIGN(32K);  

        /* 32kB for up to 16 thread user stacks */
        __thread_user_stacks_limit = .;
        . += 16 * __stack_size;
        __thread_user_stacks_base = .;

        /* 32kB for up to 16 thread kernel stacks */
        __thread_kernel_stacks_limit = .;
        . += 16 * __stack_size;
        __thread_kernel_stacks_base = .;
    } > ram
        
    __end = .;
}

