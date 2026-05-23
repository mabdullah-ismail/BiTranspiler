.386
.model flat, stdcall
.stack 4096
ExitProcess PROTO, dwExitCode:DWORD

.code
factorial PROC n:DWORD
  LOCAL result:DWORD
  LOCAL i:DWORD

  mov result, 1
  mov i, 1

L_while_start:
  mov eax, i
  cmp eax, n
  jg L_while_end

  mov eax, result
  imul eax, i
  mov result, eax

  inc i
  jmp L_while_start

L_while_end:
  mov eax, result
  ret
factorial ENDP

main PROC
  LOCAL num:DWORD
  LOCAL fact:DWORD

  mov num, 5
  push num
  call factorial
  mov fact, eax

  invoke ExitProcess, fact
main ENDP
END main
