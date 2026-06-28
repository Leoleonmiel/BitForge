include windows.inc

EXTERN GetModuleHandleW:PROC
EXTERN LoadCursorW:PROC
EXTERN LoadIconW:PROC
EXTERN RegisterClassExW:PROC
EXTERN CreateWindowExW:PROC
EXTERN ShowWindow:PROC
EXTERN UpdateWindow:PROC
EXTERN CreateSolidBrush:PROC
EXTERN WinProc:PROC
EXTERN g_bgcolor:DWORD

;variables
PUBLIC className
PUBLIC windowName
PUBLIC hInstance
PUBLIC hWndClass
PUBLIC hWindow
PUBLIC hBrush
PUBLIC windowClass


;function
PUBLIC RegisterWindowClass
PUBLIC CreateWin

.DATA
className    DW 'M','y',' ','C','l','a','s','s', 0
windowName   DW 'M','y',' ','W','i','n','d','o','w', 0
hInstance QWORD 0
hWndClass QWORD 0
hWindow QWORD 0
hBrush QWORD 0

windowClass  WNDCLASSEXW <>

.CODE
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

    ; create brush once, keep until class unregister
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
    ; Inputs: RCX=width, RDX=height, R8=x, R9=y
    ; Build params for CreateWindowExW:
    ; 1 rcx ExStyle=0
    ; 2 rdx lpClassName=&className
    ; 3 r8  lpWindowName=&windowName
    ; 4 r9  dwStyle=WS_OVERLAPPEDWINDOW
    ; 5     X
    ; 6     Y
    ; 7     nWidth
    ; 8     nHeight
    ; 9     hWndParent = 0
    ; 10    hMenu      = 0
    ; 11    hInstance  = [hInstance]
    ; 12    lpParam    = 0

    mov   r10, rcx                      ; width
    mov   r11, rdx                      ; height
    mov   r12, r8                       ; x
    mov   r13, r9                       ; y

    sub   rsp, 20h + 8*8

    mov   [rsp+20h + 0*8], r12          ; X
    mov   [rsp+20h + 1*8], r13          ; Y
    mov   [rsp+20h + 2*8], r10          ; nWidth
    mov   [rsp+20h + 3*8], r11          ; nHeight
    xor   rax, rax
    mov   [rsp+20h + 4*8], rax          ; hWndParent = 0
    mov   [rsp+20h + 5*8], rax          ; hMenu = 0
    mov   rax, [hInstance]
    mov   [rsp+20h + 6*8], rax          ; hInstance
    xor   rax, rax
    mov   [rsp+20h + 7*8], rax          ; lpParam = 0

    ; first four args in regs
    xor   ecx, ecx                      ; ExStyle = 0
    lea   rdx, className
    lea   r8,  windowName
    mov   r9d, WS_OVERLAPPEDWINDOW

    call  CreateWindowExW
    mov   [hWindow], rax

    add   rsp, 20h + 8*8
    ret
CreateWin ENDP

ShowUpdateWindow PROC
    sub   rsp, 28h
    mov   rcx, [hWindow]
    mov   edx, SW_SHOWDEFAULT
    call  ShowWindow

    mov   rcx, [hWindow]
    call  UpdateWindow
    add   rsp, 28h
    ret
ShowUpdateWindow ENDP
END