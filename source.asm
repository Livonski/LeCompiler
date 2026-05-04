format PE64 console
entry main

section '.text' code readable executable

main:
 mov dword [rsp + 20h], 69
