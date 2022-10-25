# Tri Instruction Set
This is a proof of concept I made to try out tri-color garbage
collecting. Along the way I learned about instruction sets.

Eventually I found out that actually implementing a tri-color
collector for an assembly language is difficult. Tri-color
GC's need to scan the stack for roots, but that would require
knowing the precise location of the roots. Otherwise the only
way is to conservatively scan the stack, which is what I do right
now.

The issue with stack scanning is that the GC thread would scan all
memory at the same time as the interperter.  That's a data race,
in which case I have legitimately no idea what would happen. It's
entirely possible it would get an old or new value at random, but
I don't know if it will do that and can't rely on observed behavior.

Annoyingly, that means that this project can't complete its original
purpose. I guess the true value of this project was the stuff I learned
along the way.

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
