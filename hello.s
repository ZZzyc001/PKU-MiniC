.data
.text
.globl add
add:
addi sp, sp, -32
add_entry_add:
sw a0, 28(sp)
sw a1, 24(sp)
lw t0, 28(sp)
sw t0, 20(sp)
lw t0, 24(sp)
sw t0, 16(sp)
lw t0, 20(sp)
lw t1, 16(sp)
add t0, t0, t1
sw t0, 12(sp)
lw a0, 12(sp)
addi sp, sp, 32
ret
.globl main
main:
addi sp, sp, -16
sw ra, 12(sp)
main_entry_main:
li a0, 1
li a1, 2
call add
sw a0, 8(sp)
lw a0, 8(sp)
lw ra, 12(sp)
addi sp, sp, 16
ret
