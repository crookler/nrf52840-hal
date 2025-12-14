@   file:        svc_stubs.s
@   description: Provides weaks implementations for syscalls making software pended exceptions.
@   target:      Nordic nRF52840

.syntax unified
.thumb
.section .svc_stub

@ Do nothing stub of close syscall
.thumb_func
.global _close
.type _close, %function
_close:
    bx lr
    
@ SVC with correct syscall number to invoke exit syscall
.thumb_func
.global _exit
.type _exit, %function
_exit:
    svc #3
    b . @ Infinite loop here incase for some reason we returned from the infinite loop in exit 

@ fstat syscall implementation that simply returns -1 to the caller
.thumb_func
.global _fstat
.type _fstat, %function
_fstat:
    mov r0, #0
    sub r0, r0, #1 @ Cannot load -1 into register directly
    bx lr

@ Trivial getpid syscall that just returns 1
.thumb_func
.global _getpid
.type _getpid, %function
_getpid:
    mov r0, #1
    bx lr

@ SVC with correct syscall number to invoke get_time syscall
.thumb_func
.global get_time
.type get_time, %function
get_time:
    svc #37
    bx lr

@ Do nothing stub of gettimeofday syscall
.thumb_func
.global _gettimeofday
.type _gettimeofday, %function
_gettimeofday:
    bx lr

@ Trivial isatty syscall that just returns 1
.thumb_func
.global _isatty
.type _isatty, %function
_isatty:
    mov r0, #1
    bx lr

@ Do nothing stub of kill syscall
.thumb_func
.global _kill
.type _kill, %function
_kill:
    bx lr

@ SVC with correct syscall number to invoke lock_init syscall
.thumb_func
.global lock_init
.type lock_init, %function
lock_init:
    svc #41
    bx lr

@ SVC with correct syscall number to invoke lock syscall
.thumb_func
.global lock
.type lock, %function
lock:
    svc #42
    bx lr

@ Trivial lseek syscall implementation that just returns -1
.thumb_func
.global _lseek
.type _lseek, %function
_lseek:
    mov r0, #0
    sub r0, r0, #1 @ Cannot load -1 into register directly
    bx lr

@ SVC with correct syscall number to invoke lux_read syscall
.thumb_func
.global lux_read
.type lux_read, %function
lux_read:
    svc #23
    bx lr

@ SVC with correct syscall number to invoke multitask request syscall
.thumb_func
.global multitask_request
.type multitask_request, %function
multitask_request:
    svc #31
    bx lr

@ SVC with correct syscall number to invoke multitask start syscall
.thumb_func
.global multitask_start
.type multitask_start, %function
multitask_start:
    svc #33
    bx lr

@ SVC with correct syscall number to invoke neopixel_load syscall
.thumb_func
.global neopixel_load
.type neopixel_load, %function
neopixel_load:
    svc #25
    bx lr 

@ SVC with correct syscall number to invoke neopixel_set syscall
.thumb_func
.global neopixel_set
.type neopixel_set, %function
neopixel_set:
    svc #24
    bx lr 

@ SVC with correct syscall number to invoke read syscall
.thumb_func
.global _read
.type _read, %function
_read:
    svc #2
    bx lr

@ SVC with correct syscall number to invoke sbrk syscall
.thumb_func
.global _sbrk
.type _sbrk, %function
_sbrk:
    svc #0
    bx lr

@ SVC with correct syscall number to invoke sleep_ms syscall
.thumb_func
.global sleep_ms
.type sleep_ms, %function
sleep_ms:
    svc #22
    bx lr

@ Do nothing stub of start syscall
.thumb_func
.global _start
.type _start, %function
_start:
    bx lr

@ SVC with correct syscall number to invoke set_stepper_speed syscall
.thumb_func
.global set_stepper_speed
.type set_stepper_speed, %function
set_stepper_speed:
    svc #51
    bx lr

@ SVC with correct syscall number to invoke move_stepper syscall
.thumb_func
.global move_stepper
.type move_stepper, %function
move_stepper:
    svc #52
    bx lr

@ SVC with correct syscall number to invoke thread define syscall
.thumb_func
.global thread_define
.type thread_define, %function
thread_define:
    svc #32
    bx lr

@ SVC with correct syscall number to invoke thread end syscall
.thumb_func
.global thread_end
.type thread_end, %function
thread_end:
    svc #36
    bx lr

@ SVC with correct syscall number to invoke thread id syscall
.thumb_func
.global thread_id
.type thread_id, %function
thread_id:
    svc #34
    bx lr

@ SVC with correct syscall number to invoke thread_priority syscall
.thumb_func
.global thread_priority
.type thread_priority, %function
thread_priority:
    svc #39
    bx lr

@ SVC with correct syscall number to invoke thread_time syscall
.thumb_func
.global thread_time
.type thread_time, %function
thread_time:
    svc #38
    bx lr

@ SVC with correct syscall number to invoke thread yield syscall
.thumb_func
.global thread_yield
.type thread_yield, %function
thread_yield:
    svc #35
    bx lr

@ Do nothing stub of times syscall
.thumb_func
.global _times
.type _times, %function
_times:
    bx lr

@ SVC with correct syscall number to invoke ultrasonic_read syscall
.thumb_func
.global ultrasonic_read
.type ultrasonic_read, %function
ultrasonic_read:
    svc #53
    bx lr

@ SVC with correct syscall number to invoke unlock syscall
.thumb_func
.global unlock
.type unlock, %function
unlock:
    svc #43
    bx lr

@ SVC with correct syscall number to invoke write syscall
.thumb_func
.global _write
.type _write, %function
_write:
    svc #1
    bx lr
