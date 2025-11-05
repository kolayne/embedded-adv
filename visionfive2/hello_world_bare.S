# How calls to SBI are encoded: https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/binary-encoding.adoc
# Functions to work with debug console: https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/ext-debug-console.adoc

.global _start

.section .rodata

hello_world: .string "Hello from the bare metal world!\n"

.section .text.kernel

_start:
        li a7, 0x4442434E
        li a6, 0x2          # Could have used 0x0 to print the whole string but
                            # I am trying to have fun with a loop!
        la s1, hello_world
loop:
        lb a0, 0(s1)
        beq a0, zero, shutdown
        ecall
        addi s1, s1, 1
        j loop

shutdown:
        li a7, 0x53525354  # SRST
        li a6, 0           # sbi_system_reset
        li a0, 0           # Shutdown
        li a1, 0           # No reason
        ecall

# Just in case. (should not reach here unless shutdown fails)
dead:
        wfi
        j dead
