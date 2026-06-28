include windows.inc
includelib user32.lib
includelib gdi32.lib
includelib kernel32.lib

;functions
EXTERN DefWindowProcW:PROC
EXTERN PostQuitMessage:PROC
EXTERN GetMessageW:PROC
EXTERN TranslateMessage:PROC
EXTERN DispatchMessageW:PROC
EXTERN CreateSolidBrush:PROC
EXTERN DeleteObject:PROC
EXTERN UnregisterClassW:PROC
EXTERN RegisterWindowClass:PROC
EXTERN CreateWin:PROC
EXTERN ShowWindow:PROC
EXTERN UpdateWindow:PROC

;variables
EXTERN className:WORD
EXTERN windowName:WORD
EXTERN hInstance:QWORD
EXTERN hWndClass:QWORD
EXTERN hWindow:QWORD
EXTERN hBrush:QWORD
EXTERN windowClass:WNDCLASSEXW

.DATA
message      MSG <>

.CODE
InitWindowValue PROC
    jmp InitWindow
InitWindowValue ENDP

InitWindowRef PROC
    mov rcx, [rcx]
    mov rdx, [rdx]
    mov r8,  [r8]
    mov r9,  [r9]
    jmp InitWindow
InitWindowRef ENDP

InitWindow PROC
    sub   rsp, 40h
    mov   [rsp+20h], rcx               ; width
    mov   [rsp+28h], rdx               ; height
    mov   [rsp+30h], r8                ; posX
    mov   [rsp+38h], r9                ; posY

    call  RegisterWindowClass
    test  rax, rax
    je    RunWindowFail

    mov   rcx, [rsp+20h]               ; width
    mov   rdx, [rsp+28h]               ; height
    mov   r8,  [rsp+30h]               ; x
    mov   r9,  [rsp+38h]               ; y
    call  CreateWin
    test  rax, rax
    je    RunWindowFail

    add rsp, 40h
    ret
RunWindowFail:
    xor rax, rax
    add rsp, 40h
    ret
InitWindow ENDP

DrawWindow PROC
    sub rsp, 28h    
    mov rcx, [hWindow]
    mov edx, SW_SHOWDEFAULT
    call ShowWindow

    mov rcx, [hWindow]
    call UpdateWindow

MsgLoop:
    lea  rcx, message
    xor  rdx, rdx
    xor  r8,  r8
    xor  r9,  r9
    call GetMessageW    

    cmp  eax, 0
    je   MsgDone
    cmp  eax, -1
    je   MsgDone

    lea  rcx, message
    call TranslateMessage
    lea  rcx, message
    call DispatchMessageW
    jmp  MsgLoop

MsgDone:
    add rsp, 28h
    ret
DrawWindow ENDP

WinProc PROC hWnd:QWORD, uMsg:DWORD, wParam:QWORD, lParam:QWORD
    cmp   edx, WM_DESTROY
    je    WP_Destroy

    sub   rsp, 20h
    call  DefWindowProcW
    add   rsp, 20h
    ret

WP_Destroy:
    sub   rsp, 20h
    xor   ecx, ecx
    call  PostQuitMessage
    add   rsp, 20h
    xor   rax, rax
    ret
WinProc ENDP

Unregister PROC
    sub rsp, 20h
    lea  rcx, className
    call UnregisterClassW 
    mov  rcx, [hBrush]
    test rcx, rcx
    jz   short done
    call DeleteObject
done:
    add  rsp, 20h
    ret
Unregister ENDP

END
