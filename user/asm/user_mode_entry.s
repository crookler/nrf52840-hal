@   file:        user_mode_entry.s
@   description: De-escalates current priviledge and instructs system to default to process stack instead of kernel stack.
@   target:      Nordic nRF52840

.syntax unified
.thumb
.text

@ Called to transition to unpriviledged mode
.thumb_func
.global user_mode_entry
user_mode_entry:
    @ Set the SPSEL bit (bit 1) to 1 to use user SP_process as current stack and set nPRIV (bit 0) to 1 for unpriviledged access
    mov r0, #3 
    msr control, r0
    isb @ Make sure de-escalation complete before branch followed
    bl launch_main @ Branch with linking to launch_main other helper function
