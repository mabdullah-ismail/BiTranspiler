.386
.model flat, stdcall
.stack 4096
ExitProcess PROTO, dwExitCode:DWORD

.data
  a DD 10
  b DD 5
  sum DD 0
  diff DD 0
  prod DD 0
  quot DD 0

.code
main PROC
  mov eax, a
  add eax, b
  mov sum, eax

  mov eax, a
  sub eax, b
  mov diff, eax

  mov eax, a
  imul eax, b
  mov prod, eax

  mov eax, a
  cdq
  mov ebx, b
  idiv ebx
  mov quot, eax

  mov eax, sum
  invoke ExitProcess, eax
main ENDP
END main
