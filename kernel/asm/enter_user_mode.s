@   file:        enter_user_mode.s
@   description: Loads the correct user/process stack pointer into PSP register and tail calls user_mode_entry.
@   target:      Nordic nRF52840

.syntax unified
.thumb
.text

@ Helper function to place the user stack in the correct location then branch to de-escalate priviledge
.thumb_func 
.global enter_user_mode 
enter_user_mode: 
    ldr r0, =__user_process_stack_base @ Get the address corresponding to this label
    msr psp, r0 @ Use priviledged move to put r0 insto psp (normally protected register)
    b user_mode_entry
