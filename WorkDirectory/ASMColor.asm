PUBLIC g_bgcolor
PUBLIC SetBgRGB

.DATA
g_bgcolor DWORD 00000000h

.CODE
SetBgRGB PROC r:DWORD, g:DWORD, b:DWORD
    mov eax, ecx
    and eax, 0FFh
    mov ecx, edx
    and ecx, 0FFh
    shl ecx, 8
    or eax, ecx
    mov ecx, r8d
    and ecx, 0FFh
    shl ecx, 16
    or eax, ecx
    mov [g_bgcolor], eax
    ret
SetBgRGB ENDP

SetBgColor PROC color:DWORD
    mov     [g_bgcolor], ecx
    ret
SetBgColor ENDP
END