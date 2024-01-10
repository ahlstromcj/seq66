;---------------------------------------------------------------------------
;
; File:         Seq66Constants.nsh
; Author:       Chris Ahlstrom
; Date:         2018-05-26
; Updated:      2024-01-13
; Version:      0.99.12
;
;   Provides constants commonly used by the installer for Seq66 for
;   Windows.
;
;   Note that "PRODUCT_NAME" determines the name of the directory in
;   C:/Program Files, where the application is installed.
;
;---------------------------------------------------------------------------

;============================================================================
; Product Registry keys.
;============================================================================

!define COMPANY_NAME        "Seq66"
!define PRODUCT_NAME        "Seq66"
!define PRODUCT_DIR_REGKEY  "Software\Microsoft\Windows\CurrentVersion\App Paths\qpseq66.exe"
!define PRODUCT_UNINST_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define EXE_NAME            "qpseq66.exe"

;============================================================================
; Informational settings
;============================================================================

!define VER_MAIN_PURPOSE    "Seq66 for Windows"
!define VER_NUMBER          "0.99"
!define VER_REVISION        "12"
!define VER_VARIANT         "Windows"
!define PRODUCT_VERSION     "${VER_NUMBER} ${VER_VARIANT} (rev ${VER_REVISION})"
!define PRODUCT_PUBLISHER   "C. Ahlstrom (ahlstromcj@gmail.com)"
!define PRODUCT_WEB_SITE    "https://github.com/ahlstromcj/seq66/"

;============================================================================
; The type of build to make.  Uncomment the one desired.  WIN64 preferred.
; Actually WIN32 might not be a tenable option anymore. Working on getting
; it to work (it works inside QtCreator).
;============================================================================

!define WIN64
!define WINBITS             "64"

; We currently cannot get a 32-bit build to work outside of Qtcreator.
;
; !define WIN32
; !define WINBITS           "32"

;============================================================================
; Directory to place the installer. It's in seq66/release.
;============================================================================

!define EXE_DIRECTORY       "..\release"

; vim: ts=4 sw=4 wm=3 et ft=nsis
