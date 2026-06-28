include windows.inc
includelib user32.lib
includelib gdi32.lib
includelib kernel32.lib

; =========================================
; EXTERNS
; =========================================
EXTERN DefWindowProcW:PROC
EXTERN PostQuitMessage:PROC
EXTERN GetMessageW:PROC
EXTERN PeekMessageW:PROC
EXTERN TranslateMessage:PROC
EXTERN DispatchMessageW:PROC
EXTERN CreateSolidBrush:PROC
EXTERN DeleteObject:PROC
EXTERN UnregisterClassW:PROC
EXTERN GetModuleHandleW:PROC
EXTERN LoadCursorW:PROC
EXTERN LoadIconW:PROC
EXTERN RegisterClassExW:PROC
EXTERN CreateWindowExW:PROC
EXTERN ShowWindow:PROC
EXTERN UpdateWindow:PROC
EXTERN IsWindow:PROC
EXTERN IsWindowVisible:PROC
EXTERN SetWindowTextW:PROC
EXTERN SetWindowPos:PROC

; =========================================
; CONSTANTS
; =========================================
SWP_NOSIZE      EQU 0001h
SWP_NOZORDER    EQU 0004h

; =========================================
; PUBLICS
; =========================================
PUBLIC g_bgcolor
PUBLIC SetBgRGB
PUBLIC SetBgColor
PUBLIC className
PUBLIC windowName
PUBLIC hInstance
PUBLIC hWndClass
PUBLIC hWindow
PUBLIC hBrush
PUBLIC windowClass
PUBLIC message
PUBLIC RegisterWindowClass
PUBLIC CreateWin
PUBLIC DrawWindow  
PUBLIC IsWindowOpen
PUBLIC ProcessMessages
PUBLIC WaitForClose
PUBLIC SetWindowName
PUBLIC SetWindowPosition
PUBLIC InitWindowHandle
PUBLIC GetWindowHandle


.DATA
g_bgcolor DWORD 00FFFFFFh

className           DW 'M','y',' ','C','l','a','s','s',0
defaultWindowName   DW 'M','y',' ','W','i','n','d','o','w',0
windowName          QWORD OFFSET defaultWindowName

hInstance QWORD 0
hWndClass QWORD 0
hWindow  QWORD 0
hBrush   QWORD 0

windowPos VECTOR2i <0, 0> 

windowClass  WNDCLASSEXW <>
message      MSG <>

.CODE

GetWindowHandle PROC
    mov rax, [hWindow]
    ret
GetWindowHandle ENDP

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
    mov [g_bgcolor], ecx
    ret
SetBgColor ENDP

RegisterWindowClass PROC
    sub   rsp, 20h

    xor   rcx, rcx
    call  GetModuleHandleW
    mov   [hInstance], rax

    ; clear struct
    lea   rdi, windowClass
    mov   rcx, SIZEOF WNDCLASSEXW
    xor   eax, eax
    rep   stosb

    xor   rcx, rcx
    mov   edx, IDC_ARROW
    call  LoadCursorW
    mov   windowClass.hCursor, rax

    xor   rcx, rcx
    mov   edx, IDI_APPLICATION
    call  LoadIconW
    mov   windowClass.hIcon, rax
    mov   windowClass.hIconSm, rax

    mov   windowClass.cbSize, SIZEOF WNDCLASSEXW
    mov   windowClass.style, CS_HREDRAW or CS_VREDRAW
    lea   rax, WinProc
    mov   windowClass.lpfnWndProc, rax
    mov   rax, [hInstance]
    mov   windowClass.hInstance, rax

    mov   ecx, [g_bgcolor]
    call  CreateSolidBrush
    mov   [hBrush], rax
    mov   windowClass.hbrBackground, rax

    mov   windowClass.lpszMenuName, 0
    lea   rax, className
    mov   windowClass.lpszClassName, rax

    lea   rcx, windowClass
    call  RegisterClassExW
    mov   [hWndClass], rax

    add   rsp, 20h
    ret
RegisterWindowClass ENDP

CreateWin PROC
    ; Preserve input parameters
    mov   r10, rcx        ; width
    mov   r11, rdx        ; height

    sub   rsp, 20h + 8*8  

    xor   rcx, rcx                    ; dwExStyle
    lea   rdx, className              ; lpClassName
    mov   r8,  [windowName]           ; lpWindowName
    mov   r9d, WS_OVERLAPPEDWINDOW    ; dwStyle

    mov   eax, [windowPos.x]
    mov   [rsp+20h + 0*8], eax        ; X
    mov   eax, [windowPos.y]
    mov   [rsp+20h + 1*8], eax        ; Y
    mov   eax, r10d
    mov   [rsp+20h + 2*8], eax        ; nWidth
    mov   eax, r11d
    mov   [rsp+20h + 3*8], eax        ; nHeight
    xor   rax, rax
    mov   [rsp+20h + 4*8], rax        ; hWndParent = 0
    mov   [rsp+20h + 5*8], rax        ; hMenu = 0
    mov   rax, [hInstance]
    mov   [rsp+20h + 6*8], rax        ; hInstance
    xor   rax, rax
    mov   [rsp+20h + 7*8], rax        ; lpParam = 0

    call  CreateWindowExW
    mov   [hWindow], rax

    add   rsp, 20h + 8*8
    ret
CreateWin ENDP

DrawWindow PROC
    sub   rsp, 28h
    mov   rcx, [hWindow]
    mov   edx, SW_SHOWDEFAULT
    call  ShowWindow
    mov   rcx, [hWindow]
    call  UpdateWindow
    add   rsp, 28h
    ret
DrawWindow ENDP

IsWindowOpen PROC
    sub rsp, 28h
    
    mov rcx, [hWindow]
    test rcx, rcx
    jz not_open
    
    call IsWindow
    test rax, rax
    jz not_open
    
    mov rcx, [hWindow]
    call IsWindowVisible
    test rax, rax
    jz not_open
    
    mov eax, 1
    add rsp, 28h
    ret
    
not_open:
    xor eax, eax
    add rsp, 28h
    ret
IsWindowOpen ENDP

ProcessMessages PROC
    sub rsp, 28h
    
MsgLoop:
    lea  rcx, message
    xor  rdx, rdx
    xor  r8,  r8
    xor  r9,  r9
    mov  dword ptr [rsp+20h], PM_REMOVE
    call PeekMessageW
    
    test eax, eax
    jz   MsgDone
    
    cmp  message.message, WM_QUIT
    je   MsgQuit
    
    lea  rcx, message
    call TranslateMessage
    lea  rcx, message
    call DispatchMessageW
    jmp  MsgLoop

MsgQuit:
    mov [hWindow], 0
    
MsgDone:
    add rsp, 28h
    ret
ProcessMessages ENDP

WaitForClose PROC
    sub rsp, 28h    

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
WaitForClose ENDP

InitWindowRef PROC
    mov rcx, [rcx]
    mov rdx, [rdx]
    jmp InitWindow
InitWindowRef ENDP

InitWindowHandle PROC
    sub rsp, 28h
    call InitWindow
    mov rax, [hWindow]   ; <-- FORCE RETURN
    add rsp, 28h
    ret
InitWindowHandle ENDP

InitWindow PROC
    sub   rsp, 40h
    mov   [rsp+20h], rcx        ; width
    mov   [rsp+28h], rdx        ; height

    call  RegisterWindowClass
    test  rax, rax
    je    RunWindowFail

    mov   rcx, [rsp+20h]        ; width
    mov   rdx, [rsp+28h]        ; height
    xor   r8,  r8               
    xor   r9,  r9              
    call  CreateWin
    test  rax, rax
    je    RunWindowFail

    call  DrawWindow

    mov eax, 1
    add rsp, 40h
    ret

RunWindowFail:
    xor rax, rax
    add rsp, 40h
    ret
InitWindow ENDP

WinProc PROC hWnd:QWORD, uMsg:DWORD, wParam:QWORD, lParam:QWORD
    cmp   edx, WM_DESTROY
    je    WP_Destroy
    sub   rsp, 20h
    call  DefWindowProcW
    add   rsp, 20h
    ret

WP_Destroy:
    sub   rsp, 20h
    mov   [hWindow], 0
    xor   ecx, ecx
    call  PostQuitMessage
    add   rsp, 20h
    xor   rax, rax
    ret
WinProc ENDP


SetWindowName PROC newName:QWORD
    mov [windowName], rcx
    mov rax, [hWindow]
    test rax, rax
    jz short no_window
    mov rcx, rax
    mov rdx, [windowName]
    sub rsp, 20h
    call SetWindowTextW
    add rsp, 20h
no_window:
    ret
SetWindowName ENDP

SetWindowPosition PROC posPtr:QWORD
    mov   eax, [rcx]            ; x (32-bit)
    mov   [windowPos.x], eax
    mov   eax, [rcx+4]          ; y (32-bit)
    mov   [windowPos.y], eax

    mov   rax, [hWindow]
    test  rax, rax
    jz    short no_window 

    sub   rsp, 40h
    mov   rcx, rax             
    xor   rdx, rdx              
    mov   r8d, [windowPos.x]    ; X
    mov   r9d, [windowPos.y]    ; Y
    mov   dword ptr [rsp+20h], 0       ; nWidth
    mov   dword ptr [rsp+28h], 0       ; nHeight
    mov   dword ptr [rsp+30h], SWP_NOSIZE or SWP_NOZORDER or SWP_SHOWWINDOW
    call  SetWindowPos
    add   rsp, 40h

no_window:
    ret
SetWindowPosition ENDP

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
