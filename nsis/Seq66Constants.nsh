;---------------------------------------------------------------------------
;
; File:         Seq66Constants.nsh
; Author:       Chris Ahlstrom
; Date:         2018-05-26
; Updated:      2021-12-12
; Version:      0.98.2
;
;   Provides constants commonly used by the installer for Seq66 for
;   Windows.
;
;   Note that "PRODUCT_NAME" determines the name of the directory in
;   C:/Program Files(x86) where the application is installed.
;
;---------------------------------------------------------------------------

;============================================================================
; Product Registry keys.
;============================================================================

!define PRODUCT_NAME        "Seq66"
!define PRODUCT_DIR_REGKEY  "Software\Microsoft\Windows\CurrentVersion\App Paths\qpseq66.exe"
!define PRODUCT_UNINST_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY     "HKLM"

;============================================================================
; Informational settings
;============================================================================

!define VER_MAIN_PURPOSE    "Seq66 for Windows"
!define VER_NUMBER          "0.98"
!define VER_REVISION        "2"
!define VER_VARIANT         "Windows"
!define PRODUCT_VERSION     "${VER_NUMBER} ${VER_VARIANT} (rev ${VER_REVISION})"
!define PRODUCT_PUBLISHER   "C. Ahlstrom (ahlstromcj@gmail.com)"
!define PRODUCT_WEB_SITE    "https://github.com/ahlstromcj/seq66/"

;============================================================================
; Directory to place the installer.
;============================================================================

!define EXE_DIRECTORY       "..\release"

; vim: ts=4 sw=4 wm=3 et ft=nsis
