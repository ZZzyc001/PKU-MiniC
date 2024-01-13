.data
.globl buf
buf:
.zero 800
.text
.globl merge_sort
merge_sort:
addi sp, sp, -416
sw ra, 412(sp)
merge_sort_entry_merge_sort:
sw a0, 408(sp)
sw a1, 404(sp)
lw t0, 408(sp)
sw t0, 400(sp)
lw t0, 400(sp)
li t1, 1
add t0, t0, t1
sw t0, 396(sp)
lw t0, 404(sp)
sw t0, 392(sp)
lw t0, 396(sp)
lw t1, 392(sp)
slt t0, t1, t0
sw t0, 388(sp)
lw t0, 388(sp)
beqz t0, merge_sort_skip0
j merge_sort_true_0
merge_sort_skip0:
j merge_sort_false_0
merge_sort_true_0:
lw ra, 412(sp)
addi sp, sp, 416
ret
merge_sort_false_0:
j merge_sort_end_0
merge_sort_end_0:
lw t0, 408(sp)
sw t0, 384(sp)
lw t0, 404(sp)
sw t0, 380(sp)
lw t0, 384(sp)
lw t1, 380(sp)
add t0, t0, t1
sw t0, 376(sp)
lw t0, 376(sp)
li t1, 2
div t0, t0, t1
sw t0, 372(sp)
lw t0, 372(sp)
sw t0, 368(sp)
lw t0, 408(sp)
sw t0, 364(sp)
lw t0, 368(sp)
sw t0, 360(sp)
lw a0, 364(sp)
lw a1, 360(sp)
call merge_sort
lw t0, 368(sp)
sw t0, 356(sp)
lw t0, 404(sp)
sw t0, 352(sp)
lw a0, 356(sp)
lw a1, 352(sp)
call merge_sort
lw t0, 408(sp)
sw t0, 348(sp)
lw t0, 348(sp)
sw t0, 344(sp)
lw t0, 368(sp)
sw t0, 340(sp)
lw t0, 340(sp)
sw t0, 336(sp)
lw t0, 408(sp)
sw t0, 332(sp)
lw t0, 332(sp)
sw t0, 328(sp)
j merge_sort_while_entry_3
merge_sort_while_entry_3:
li t0, 0
sw t0, 324(sp)
lw t0, 344(sp)
sw t0, 320(sp)
lw t0, 368(sp)
sw t0, 316(sp)
lw t0, 320(sp)
lw t1, 316(sp)
slt t0, t0, t1
sw t0, 312(sp)
lw t0, 312(sp)
li t1, 0
xor t0, t0, t1
snez t0, t0
sw t0, 308(sp)
lw t0, 308(sp)
beqz t0, merge_sort_skip1
j merge_sort_true_1
merge_sort_skip1:
j merge_sort_end_1
merge_sort_true_1:
lw t0, 336(sp)
sw t0, 304(sp)
lw t0, 404(sp)
sw t0, 300(sp)
lw t0, 304(sp)
lw t1, 300(sp)
slt t0, t0, t1
sw t0, 296(sp)
lw t0, 296(sp)
li t1, 0
xor t0, t0, t1
snez t0, t0
sw t0, 292(sp)
lw t0, 292(sp)
sw t0, 324(sp)
j merge_sort_end_1
merge_sort_end_1:
lw t0, 324(sp)
sw t0, 288(sp)
lw t0, 288(sp)
beqz t0, merge_sort_skip2
j merge_sort_while_body_3
merge_sort_skip2:
j merge_sort_end_3
merge_sort_while_body_3:
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 284(sp)
lw t0, 344(sp)
sw t0, 280(sp)
addi t0, sp, 284
lw t0, 0(t0)
lw t1, 280(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 276(sp)
lw t0, 276(sp)
lw t0, 0(t0)
sw t0, 272(sp)
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 268(sp)
lw t0, 336(sp)
sw t0, 264(sp)
addi t0, sp, 268
lw t0, 0(t0)
lw t1, 264(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 260(sp)
lw t0, 260(sp)
lw t0, 0(t0)
sw t0, 256(sp)
lw t0, 272(sp)
lw t1, 256(sp)
slt t0, t0, t1
sw t0, 252(sp)
lw t0, 252(sp)
beqz t0, merge_sort_skip3
j merge_sort_true_2
merge_sort_skip3:
j merge_sort_false_2
merge_sort_true_2:
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 248(sp)
lw t0, 344(sp)
sw t0, 244(sp)
addi t0, sp, 248
lw t0, 0(t0)
lw t1, 244(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 240(sp)
lw t0, 240(sp)
lw t0, 0(t0)
sw t0, 236(sp)
la t0, buf
li t1, 1
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 232(sp)
lw t0, 328(sp)
sw t0, 228(sp)
addi t0, sp, 232
lw t0, 0(t0)
lw t1, 228(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 224(sp)
lw t1, 224(sp)
lw t0, 236(sp)
sw t0, 0(t1)
lw t0, 344(sp)
sw t0, 220(sp)
lw t0, 220(sp)
li t1, 1
add t0, t0, t1
sw t0, 216(sp)
lw t0, 216(sp)
sw t0, 344(sp)
j merge_sort_end_2
merge_sort_false_2:
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 212(sp)
lw t0, 336(sp)
sw t0, 208(sp)
addi t0, sp, 212
lw t0, 0(t0)
lw t1, 208(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 204(sp)
lw t0, 204(sp)
lw t0, 0(t0)
sw t0, 200(sp)
la t0, buf
li t1, 1
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 196(sp)
lw t0, 328(sp)
sw t0, 192(sp)
addi t0, sp, 196
lw t0, 0(t0)
lw t1, 192(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 188(sp)
lw t1, 188(sp)
lw t0, 200(sp)
sw t0, 0(t1)
lw t0, 336(sp)
sw t0, 184(sp)
lw t0, 184(sp)
li t1, 1
add t0, t0, t1
sw t0, 180(sp)
lw t0, 180(sp)
sw t0, 336(sp)
j merge_sort_end_2
merge_sort_end_2:
lw t0, 328(sp)
sw t0, 176(sp)
lw t0, 176(sp)
li t1, 1
add t0, t0, t1
sw t0, 172(sp)
lw t0, 172(sp)
sw t0, 328(sp)
j merge_sort_while_entry_3
merge_sort_end_3:
j merge_sort_while_entry_4
merge_sort_while_entry_4:
lw t0, 344(sp)
sw t0, 168(sp)
lw t0, 368(sp)
sw t0, 164(sp)
lw t0, 168(sp)
lw t1, 164(sp)
slt t0, t0, t1
sw t0, 160(sp)
lw t0, 160(sp)
beqz t0, merge_sort_skip4
j merge_sort_while_body_4
merge_sort_skip4:
j merge_sort_end_4
merge_sort_while_body_4:
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 156(sp)
lw t0, 344(sp)
sw t0, 152(sp)
addi t0, sp, 156
lw t0, 0(t0)
lw t1, 152(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 148(sp)
lw t0, 148(sp)
lw t0, 0(t0)
sw t0, 144(sp)
la t0, buf
li t1, 1
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 140(sp)
lw t0, 328(sp)
sw t0, 136(sp)
addi t0, sp, 140
lw t0, 0(t0)
lw t1, 136(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 132(sp)
lw t1, 132(sp)
lw t0, 144(sp)
sw t0, 0(t1)
lw t0, 344(sp)
sw t0, 128(sp)
lw t0, 128(sp)
li t1, 1
add t0, t0, t1
sw t0, 124(sp)
lw t0, 124(sp)
sw t0, 344(sp)
lw t0, 328(sp)
sw t0, 120(sp)
lw t0, 120(sp)
li t1, 1
add t0, t0, t1
sw t0, 116(sp)
lw t0, 116(sp)
sw t0, 328(sp)
j merge_sort_while_entry_4
merge_sort_end_4:
j merge_sort_while_entry_5
merge_sort_while_entry_5:
lw t0, 336(sp)
sw t0, 112(sp)
lw t0, 404(sp)
sw t0, 108(sp)
lw t0, 112(sp)
lw t1, 108(sp)
slt t0, t0, t1
sw t0, 104(sp)
lw t0, 104(sp)
beqz t0, merge_sort_skip5
j merge_sort_while_body_5
merge_sort_skip5:
j merge_sort_end_5
merge_sort_while_body_5:
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 100(sp)
lw t0, 336(sp)
sw t0, 96(sp)
addi t0, sp, 100
lw t0, 0(t0)
lw t1, 96(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 92(sp)
lw t0, 92(sp)
lw t0, 0(t0)
sw t0, 88(sp)
la t0, buf
li t1, 1
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 84(sp)
lw t0, 328(sp)
sw t0, 80(sp)
addi t0, sp, 84
lw t0, 0(t0)
lw t1, 80(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 76(sp)
lw t1, 76(sp)
lw t0, 88(sp)
sw t0, 0(t1)
lw t0, 336(sp)
sw t0, 72(sp)
lw t0, 72(sp)
li t1, 1
add t0, t0, t1
sw t0, 68(sp)
lw t0, 68(sp)
sw t0, 336(sp)
lw t0, 328(sp)
sw t0, 64(sp)
lw t0, 64(sp)
li t1, 1
add t0, t0, t1
sw t0, 60(sp)
lw t0, 60(sp)
sw t0, 328(sp)
j merge_sort_while_entry_5
merge_sort_end_5:
j merge_sort_while_entry_6
merge_sort_while_entry_6:
lw t0, 408(sp)
sw t0, 56(sp)
lw t0, 404(sp)
sw t0, 52(sp)
lw t0, 56(sp)
lw t1, 52(sp)
slt t0, t0, t1
sw t0, 48(sp)
lw t0, 48(sp)
beqz t0, merge_sort_skip6
j merge_sort_while_body_6
merge_sort_skip6:
j merge_sort_end_6
merge_sort_while_body_6:
la t0, buf
li t1, 1
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 44(sp)
lw t0, 408(sp)
sw t0, 40(sp)
addi t0, sp, 44
lw t0, 0(t0)
lw t1, 40(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 36(sp)
lw t0, 36(sp)
lw t0, 0(t0)
sw t0, 32(sp)
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 28(sp)
lw t0, 408(sp)
sw t0, 24(sp)
addi t0, sp, 28
lw t0, 0(t0)
lw t1, 24(sp)
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 20(sp)
lw t1, 20(sp)
lw t0, 32(sp)
sw t0, 0(t1)
lw t0, 408(sp)
sw t0, 16(sp)
lw t0, 16(sp)
li t1, 1
add t0, t0, t1
sw t0, 12(sp)
lw t0, 12(sp)
sw t0, 408(sp)
j merge_sort_while_entry_6
merge_sort_end_6:
lw ra, 412(sp)
addi sp, sp, 416
ret
.globl main
main:
addi sp, sp, -48
sw ra, 44(sp)
main_entry_main:
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 40(sp)
addi t0, sp, 40
lw t0, 0(t0)
li t1, 0
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 36(sp)
lw a0, 36(sp)
call getarray
sw a0, 32(sp)
lw t0, 32(sp)
sw t0, 28(sp)
lw t0, 28(sp)
sw t0, 24(sp)
li a0, 0
lw a1, 24(sp)
call merge_sort
lw t0, 28(sp)
sw t0, 20(sp)
la t0, buf
li t1, 0
li t2, 400
mul t1, t1, t2
add t0, t0, t1
sw t0, 16(sp)
addi t0, sp, 16
lw t0, 0(t0)
li t1, 0
li t2, 4
mul t1, t1, t2
add t0, t0, t1
sw t0, 12(sp)
lw a0, 20(sp)
lw a1, 12(sp)
call putarray
li a0, 0
lw ra, 44(sp)
addi sp, sp, 48
ret
