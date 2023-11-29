#define ASM     1

.globl execute_sigreturn
.globl execute_sigreturn_end

execute_sigreturn:
    movl $10, %eax
    int $0x80
    RET
execute_sigreturn_end: