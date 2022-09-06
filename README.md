# Tri Instruction Set
This is a proof of concept I made to try out tri-color garbage
collecting. Along the way I learned about instruction sets

## ISA reference
There are 16 registers:  
ip is the Instruction Pointer  
bp is the Base Pointer  
sp is the Stack Pointer  
rp is the Return Pointer  
r0 - r11 are general use registers  

[bracketed] operands are registers while
<arrow> braced operands are either registers,
or immediate values.  
hlt ends execution  
addi [a] + [b] = [c]  
subi [a] - [b] = [c]  
muli [a] + [b] = [c]  
divi [a] / [b] = [c]  
mov  [a]-> [b]  
out prints [a]  
jmp goto [a]  
jnz if [a]==0 goto [b]  
jez if [a]!=0 goto [b]  
call goto [a] and store (ip) into (rp)  
alloc allocate [a] storage and put the pointer into [b]  
noop does absolutely nothing