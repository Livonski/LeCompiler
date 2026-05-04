format PE64 console
entry main

section '.text' code readable executable

main:
 mov eax, 34
 mov dword [rsp + 20h], eax
 mov eax, 35
 mov dword [rsp + 24h], eax
 mov eax, dword [rsp + 20h]
 mov ebx, dword [rsp + 24h]
 add eax, ebx
 mov ecx, eax
 call [ExitProcess]
section '.idata' import data readable writeable

dd 0, 0, 0, RVA kernel32_name, RVA kernel32_table
dd 0, 0, 0, 0, 0

kernel32_table:
 ExitProcess dq RVA _ExitProcess
 dq 0

kernel32_name db 'kernel32.dll', 0

_ExitProcess:
 dw 0
 db 'ExitProcess', 0
