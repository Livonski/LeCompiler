format PE64 console
entry _start

section '.text' code readable executable


_start:
    sub rsp, 48h

    mov ecx, -11
    call [GetStdHandle]
    mov [stdout_handle], rax

    call le_main

    mov [return_code], eax

    add eax, '0'
    mov [return_digit], al

    mov rcx, [stdout_handle]
    lea rdx, [exit_message]
    mov r8d, exit_message_len
    lea r9, [written]
    mov qword [rsp + 20h], 0
    call [WriteConsoleA]
    mov ecx, [return_code]
    call [ExitProcess]

le_main:
    sub rsp, 108h
    mov eax, 10
    mov dword [rsp + 20h], eax
    mov eax, 20
    mov dword [rsp + 24h], eax
    mov dword [rsp + 28h], 3
repeat_0:
    cmp dword [rsp + 28h], 0
    jle repeat_end_0
    mov eax, 30
    mov dword [rsp + 2Ch], eax
    mov eax, 40
    mov dword [rsp + 30h], eax
    dec dword [rsp + 28h]
    jmp repeat_0
repeat_end_0:
    mov dword [rsp + 2Ch], 5
repeat_1:
    cmp dword [rsp + 2Ch], 0
    jle repeat_end_1
    mov eax, 50
    mov dword [rsp + 30h], eax
    mov eax, 60
    mov dword [rsp + 34h], eax
    dec dword [rsp + 2Ch]
    jmp repeat_1
repeat_end_1:
    mov eax, dword [rsp + 20h]
    add rsp, 108h
    ret
section '.data' data readable writeable

exit_message db 'program exited with return code '
return_digit db '0'
db 13, 10
exit_message_len = $ - exit_message

return_code dd 0
stdout_handle dq 0
char_buffer db 0
written dd 0

section '.idata' import data readable writeable

dd 0, 0, 0, RVA kernel32_name, RVA kernel32_table
dd 0, 0, 0, 0, 0

kernel32_table:
 ExitProcess dq RVA _ExitProcess
 GetStdHandle dq RVA _GetStdHandle
 WriteConsoleA dq RVA _WriteConsoleA
 dq 0

kernel32_name db 'kernel32.dll', 0

_ExitProcess:
 dw 0
 db 'ExitProcess', 0

_GetStdHandle:
 dw 0
 db 'GetStdHandle', 0

_WriteConsoleA:
 dw 0
 db 'WriteConsoleA', 0

