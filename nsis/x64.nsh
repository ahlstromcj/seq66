;---------------------------------------------------------------------------
;
; File:         x64.nsh
; Author:       Chris Ahlstrom
; Date:         2021-12-12
; Updated:      2021-12-12
; Version:      0.98.2
;
;   NSIS, presently a 32-bit program, redirects installations to default
;    "C:\Program Files (x86)" and uses the Windows Registry key
;   "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node".
;
;   The "x64" plug-in provides a macro, "RunningX64", that returns true when
;   the installer runs on 64-bit Windows under WoW64 emulation.  It can be
;   obtained from: "http://nsis.sourceforge.net/Include/x64.nsh".
;
;   This version has comments added and perhaps some tweaks. It has a few
;   simple macros to handle installations on x64 machines.
;
;   -   RunningX64 checks if the installer is running on a 64-bit OS.
;   -   IsWow64 checks if the installer is 32-bit, running on a 64-bit OS.
;   -   IsNative*** checks the OS native CPU architecture.
;   -   DisableX64FSRedirection disables file system redirection.
;   -   EnableX64FSRedirection enables file system redirection.
;
;   Note that this script requires (we think) NSIS 3.0 and above.
;
; Usages:
;
;   ${If} ${RunningX64}
;     MessageBox MB_OK "Running on 64-bit Windows"
;   ${EndIf}
;
;   ${If} ${IsNativeAMD64}
;     ; Install AMD64 64-bit driver/library
;   ${ElseIf} ${IsNativeARM64}
;     ; Install ARM64 64-bit driver/library
;   ${ElseIf} ${IsNativeIA32}
;     ; Install i386 32-bit driver/library
;   ${Else}
;     Abort "Unsupported CPU architecture!"
;   ${EndIf}
;
;   ${If} ${IsNativeAMD64}
;       File "amd64\myapp.exe"
;   ${ElseIf} ${IsNativeIA32}
;   ${OrIf} ${IsWow64}
;       File "x86\myapp.exe"
;   ${Else}
;       Abort "Unsupported CPU architecture!"
;   ${EndIf}
;
;   SetOutPath $SYSDIR
;   ${DisableX64FSRedirection}
;   File something.bin # extracts to C:\Windows\System32
;   ${EnableX64FSRedirection}
;   File something.bin # extracts to C:\Windows\SysWOW64
;
;---------------------------------------------------------------------------

!ifndef ___X64__NSH___
!define ___X64__NSH___

!include LogicLib.nsh       ; /usr/share/nsis/Include/LogicLib.nsh

;---------------------------------------------------------------------------
; IsWow64()
;
;   1. [Win10.1511+] 0 if not WOW64
;   2. [WinXP+] FALSE for a 32-bit application on ARM64!
;
;---------------------------------------------------------------------------

!define IsWow64 `"" IsWow64 ""`
!macro _IsWow64 _a _b _t _f
    !insertmacro _LOGICLIB_TEMP
    System::Call kernel32::GetCurrentProcess()p.s
    System::Call kernel32::IsWow64Process2(ps,*i0s,*i)    ; Note 1
    Push |
    System::Call kernel32::IsWow64Process(p-1,*i0s)       ; Note 2
    System::Int64Op
    Pop $_LOGICLIB_TEMP
    !insertmacro _!= $_LOGICLIB_TEMP 0 `${_t}` `${_f}`
!macroend

;---------------------------------------------------------------------------
; RunningX64()
;---------------------------------------------------------------------------

!define RunningX64 `"" RunningX64 ""`
!macro _RunningX64 _a _b _t _f 
    !if ${NSIS_PTR_SIZE} > 4
        !insertmacro LogicLib_JumpToBranch `${_t}` `${_f}`
    !else
        !insertmacro _IsWow64 `${_a}` `${_b}` `${_t}` `${_f}`
    !endif
!macroend

;---------------------------------------------------------------------------
; GetNativeMachineArchitecture()
;
;   1. Always IMAGE_FILE_MACHINE_I386 on Win9x.
;
;---------------------------------------------------------------------------

!define GetNativeMachineArchitecture "!insertmacro GetNativeMachineArchitecture "
!macro GetNativeMachineArchitecture outvar
    !define GetNativeMachineArchitecture_lbl lbl_GNMA_${__COUNTER__}
    System::Call kernel32::GetCurrentProcess()p.s
    System::Call kernel32::IsWow64Process2(ps,*i,*i0s)
    Pop ${outvar}
    IntCmp ${outvar} 0 "" ${GetNativeMachineArchitecture_lbl}_done ${GetNativeMachineArchitecture_lbl}_done
        !if "${NSIS_PTR_SIZE}" <= 4
        !if "${NSIS_CHAR_SIZE}" <= 1
        System::Call 'USER32::CharNextW(w"")p.s'
        Pop ${outvar}
        IntPtrCmpU ${outvar} 0 "" ${GetNativeMachineArchitecture_lbl}_oldnt ${GetNativeMachineArchitecture_lbl}_oldnt
            StrCpy ${outvar} 332            ; Note 1
            Goto ${GetNativeMachineArchitecture_lbl}_done
        ${GetNativeMachineArchitecture_lbl}_oldnt:
        !endif
        !endif
        System::Call '*0x7FFE002E(&i2.s)'
        Pop ${outvar}
    ${GetNativeMachineArchitecture_lbl}_done:
    !undef GetNativeMachineArchitecture_lbl
!macroend

;---------------------------------------------------------------------------
; _IsNativeMachineArchitecture()
;---------------------------------------------------------------------------

!macro _IsNativeMachineArchitecture _ignore _arc _t _f
    !insertmacro _LOGICLIB_TEMP
    ${GetNativeMachineArchitecture} $_LOGICLIB_TEMP
    !insertmacro _= $_LOGICLIB_TEMP ${_arc} `${_t}` `${_f}`
!macroend

!define IsNativeMachineArchitecture `"" IsNativeMachineArchitecture `
!define IsNativeIA32 '${IsNativeMachineArchitecture} 332'       ; Intel x86
!define IsNativeAMD64 '${IsNativeMachineArchitecture} 34404'    ; x86-64/x64
!define IsNativeARM64 '${IsNativeMachineArchitecture} 43620'    ; ARM 64

;---------------------------------------------------------------------------
; DisableX64FSRedirection()
;---------------------------------------------------------------------------

!define DisableX64FSRedirection "!insertmacro DisableX64FSRedirection"
!macro DisableX64FSRedirection
    System::Call kernel32::Wow64EnableWow64FsRedirection(i0)
!macroend

;---------------------------------------------------------------------------
; EnableX64FSRedirection()
;---------------------------------------------------------------------------

!define EnableX64FSRedirection "!insertmacro EnableX64FSRedirection"
!macro EnableX64FSRedirection
    System::Call kernel32::Wow64EnableWow64FsRedirection(i1)
!macroend

!endif  # !___X64__NSH___

; vim: ts=4 sw=4 wm=3 et ft=nsis
