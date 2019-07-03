.intel_syntax noprefix
.global _main
.LC0:
  .string "int * = %d, char * = %d, int = %d, char = %d\n"
_main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov r10, 0
  mov dword ptr [rbp-4], r10d
  mov r10, 1
  mov r8b, r10b
  mov r10, 4
  mov ecx, r10d
  mov r10, 8
  mov rdx, r10
  mov r10, 8
  mov rsi, r10
  lea r10, [rip+.LC0]
  mov rdi, r10
  call _printf
  mov r10, rax
  mov r11, 0
  add rsp, 16
  mov rax, r11
  leave
  ret
