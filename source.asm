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
    mov eax, 72
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 101
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 108
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 108
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 111
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 87
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 111
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 114
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 108
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 100
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 33
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 10
    mov [char_buffer], al

    mov rcx, [stdout_handle]
    lea rdx, [char_buffer]
    mov r8d, 1
    lea r9, [written]
    mov qword [rsp+20h], 0
    call [WriteConsoleA]

    mov eax, 1
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

