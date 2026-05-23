.386
.model flat, stdcall
.stack 4096
ExitProcess PROTO, dwExitCode:DWORD

.code
fibonacci PROC n:DWORD
  LOCAL prev2:DWORD
  LOCAL prev1:DWORD
  LOCAL current:DWORD

  cmp n, 1
  jg L_if_end
  mov eax, n
  ret

L_if_end:
  mov prev2, 0
  mov prev1, 1
  mov current, 0

  mov ecx, n
  dec ecx ; loop count n-1
L_loop:
  mov eax, prev1
  add eax, prev2
  mov current, eax

  mov eax, prev1
  mov prev2, eax

  mov eax, current
  mov prev1, eax
  loop L_loop

  mov eax, current
  ret
fibonacci ENDP

main PROC
  push 7
  call fibonacci
  invoke ExitProcess, eax
main ENDP
END main
