;---------------------------------------------------------------------------
;
; File:         Seq66Setup.nsi
; Author:       Chris Ahlstrom
; Date:         2018-05-26
; Updated:      2023-10-24
; Version:      0.99.10
;
; Usage of this Windows build script:
;
;    -  See the build_release_package.bat file for full details.
;    -  Obtain and install the NSIS 2.46 (or above) installer from
;       http://nsis.sourceforge.net/Download, or preferably install it from
;       your Linux repository via apt. It can also be installed on
;       Windows, and the build script can detect if it is available on
;       the PATH.
;    -  In Windows, Check out the latest branch project from Git.  Or
;       make a source package using the handy "pack" script on Linux,
;       and copy the source package to your Windows system, and unpack it
;       there.
;    -  In the project directory, on the command-line, run the following
;       command to build the Release version of Seq66 using qmake and make,
;       and to create a 7-Zip "release" package that can be unpacked in
;       the root "seq66" directory.
;           C:\Projects\seq66\build_release_package.bat
;    -  The resulting package is something like the file
;       "qpseq66-release-package-0.95.1.7z", found in
;       ../seq66/seq66-release/Seq66qt5.
;    -  Then run NSIS:
;       -   Windows:
;           -   Click on "Compile NSI scripts".
;           -   Click File / Load Script.
;           -   Navigate to the "nsis" directory and select
;               "Seq66Setup_V0.95.nsi".  The script will take a few minutes
;               to build.  The output goes to ".... (TBD)"
;           -   You can run that executable, or you can instead click the
;               "Test Installer" button in the NSIS window.
;           -   When you get to the "Choose Install Location" window, you can
;               use "C" and test the installation.
;           -   Or, as in Linux, the makensis command can be used if
;               available.
;       -   Linux: The program that creates Windows installers on Linux is
;           'makensis'.
;           -   The actual build is done on Windows.
;           -   Change to the "seq66/nsis" directory.
;           -   Run "makensis Seq66Setup.nsi".
;    -  After creation, The installer package is at
;       "seq66/release/seq66_setup_x64-0.99.5.exe" or similar.
;       -   Select the defaults and let the installer do its thing.
;    -  To uninstall the application, use Settings /
;           Control Panel / Add and Remove Programs.  The application is
;           Seq66, and the executable is qpseq66.exe.
;
; References:
;
;    -  http://nsis.sourceforge.net/Download
;    -  http://www.atomicmpc.com.au/Feature/24263,
;           tutorial-create-a-nsis-install-script.aspx/2
;
;---------------------------------------------------------------------------

;---------------------------------------------------------------------------
;   MUI.nsh provides GUI features as expected for an installer.
;   Sections.nsh provides support for sections and section groups.
;   Seq66Constants.nsh contains names and version numbers.
;---------------------------------------------------------------------------

Unicode True

!include MUI.nsh
!include MUI2.nsh
!include Sections.nsh
!include Seq66Constants.nsh

!define MUI_ICON "..\resources\icons\route66.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "..\resources\icons\route66.bmp"
!define MUI_HEADERIMAGE_RIGHT

;---------------------------------------------------------------------------
; We want:
;
;   -   The description at the bottom.
;   -   A welcome page.
;   -   A license page.
;   -   A components page.
;   -   A directory page to allow changing the installation location of
;       Seq66.
;   -   An install-files page.
;   -   A finish page.
;   -   An uninstaller page.
;   -   An abort-warning prompt.
;
;---------------------------------------------------------------------------

!define MUI_COMPONENTSPAGE_SMALLDESC
!insertmacro MUI_PAGE_WELCOME

!define MUI_LICENSEPAGE_CHECKBOX
!insertmacro MUI_PAGE_LICENSE "..\data\license.text"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

;---------------------------------------------------------------------------
; Tentative code to install a desktop icon. Disabls the readme prompt and
; uses it to prompt for installing a desktop icon
;
; Function finishpageaction
; CreateShortcut "$DESKTOP\qpseq66.lnk" "$iNSTDIR\qpseq66.exe"
; FunctionEnd
;
; !define MUI_FINISHPAGE_SHOWREADME ""
; !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
; !define MUI_FINISHPAGE_SHOWREADME_TEXT "Create Desktop Shortcut"
; !define MUI_FINISHPAGE_SHOWREADME_FUNCTION finishpageaction
;
;----------------------------
;
; Uninstall:
;
; Removes pins using the shortcut.
; Deletes the shortcut.
; Refreshes the desktop.
;
; !macro customInstall WinShell::UninstShortcut "$desktopLink"
;
; Delete "$desktopLink"
; System::Call 'shell32::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
;
; !macroend
;
;---------------------------------------------------------------------------

!define MUI_FINISHPAGE_SHOWREADME "..\data\readme.text"
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_INSTFILES

!define MUI_ABORTWARNING
!insertmacro MUI_LANGUAGE "English"

;---------------------------------------------------------------------------
; Here we set a non-silent install.
;
;   SilentInstall silent
;
;---------------------------------------------------------------------------

SilentInstall normal

Name "${PRODUCT_NAME} ${PRODUCT_VERSION} ${WINBITS}-bit"
BrandingText "${PRODUCT_NAME} ${PRODUCT_VERSION} NSIS-based Installer"
OutFile "${EXE_DIRECTORY}\seq66_setup_x${WINBITS}-${VER_NUMBER}.${VER_REVISION}.exe"
RequestExecutionLevel admin
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
InstallDir "$PROGRAMFILES${WINBITS}\${PRODUCT_NAME}"

;---------------------------------------------------------------------------
; The actual installer sections
;---------------------------------------------------------------------------

Section "Application" SEC_APPLIC

    SetOutPath "$INSTDIR"
    SetOverwrite on
    File "..\release\qpseq66.exe"

SectionEnd

SectionGroup "Qt5 Support" SEC_QT5

Section "Mingw DLLs" SEC_MINGW

    SetOutPath "$INSTDIR"
    SetOverwrite on
    File "..\release\D3Dcompiler_47.dll"
    File "..\release\lib*.dll"
    File "..\release\opengl*.dll"

SectionEnd

Section "Qt5 Main DLLs" SEC_QTDLLS

    SetOutPath "$INSTDIR"
    SetOverwrite on
    File "..\release\Qt*.dll"

SectionEnd

Section "Qt5 Icon Engine" SEC_QTICON

    SetOutPath "$INSTDIR\iconengines"
    SetOverwrite on
    File /r "..\release\iconengines\*.*"

SectionEnd

Section "Qt5 Imaging" SEC_QTIMG

    SetOutPath "$INSTDIR\imageformats"
    SetOverwrite on
    File /r "..\release\imageformats\*.*"

SectionEnd

Section "Qt5 Platform Support" SEC_QTPLAT

    SetOutPath "$INSTDIR\platforms"
    SetOverwrite on
    File /r "..\release\platforms\*.*"

SectionEnd

Section "Qt5 Style Engine" SEC_QTSTYLE

    SetOutPath "$INSTDIR\styles"
    SetOverwrite on
    File /r "..\release\styles\*.*"

SectionEnd

Section "Qt5 Translations" SEC_QTTRANS

    SetOutPath "$INSTDIR\translations"
    SetOverwrite on
    File /r "..\release\translations\*.*"

SectionEnd

SectionGroupEnd

Section "Licensing and Sample Files" SEC_LIC

    SetOutPath "$INSTDIR\data"
    SetOverwrite on
    File /r "..\release\data\*"

SectionEnd

Section "Documentation" SEC_DOC

    SetOutPath "$INSTDIR\doc"
    SetOverwrite on
    File /r "..\release\data\share\doc\*.pdf"
    File /r "..\release\data\share\doc\*.ods"
    File /r "..\release\data\share\doc\tutorial\*.*"
    File /r "..\release\data\share\doc\info\*.*"

SectionEnd

;--------------------------------------------------------------------------
; Section "Registry Entries"
;
;   Seq66 is completely configured via qpseq66.rc and qpseq66.usr
;   in the user-directory C:/Users/username/AppData/Local/seq66.
;
;--------------------------------------------------------------------------

; Section "Registry Entries"
; SectionEnd

;--------------------------------------------------------------------------
; Post
;--------------------------------------------------------------------------
;
;   In this section, uninstallation Registry keys are added.
;
;   We are not sure if they are needed.
;
; https://nsis.sourceforge.io/
;   A_simple_installer_with_start_menu_shortcut_and_uninstaller
;
;--------------------------------------------------------------------------

Section -Post

    SetRegView ${WINBITS}

; EXE_NAME is just a constant that is defined in the script, you can just use
;
; CreateShortcut "$smprograms\my app\my shortcut.lnk"
;     "c:\path\to\application.exe" "" "c:\path\to\application.exe" 0
;
; It will create a shortcut to the application executable.

    CreateDirectory '$SMPROGRAMS\${COMPANY_NAME}\${PRODUCT_NAME}'
    CreateShortCut '$SMPROGRAMS\${COMPANY_NAME}\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk' \
        '$INSTDIR\${EXE_NAME}' "" '$INSTDIR\${EXE_NAME}' 0
    CreateShortCut '$SMPROGRAMS\${COMPANY_NAME}\${PRODUCT_NAME}\Uninstall ${PRODUCT_NAME}.lnk' \
        '$INSTDIR\uninst.exe' "" '$INSTDIR\uninst.exe' 0

    WriteUninstaller "$INSTDIR\uninst.exe"
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\qpseq66.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\qpseq66.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"

SectionEnd

;--------------------------------------------------------------------------
; Section Descriptions
;--------------------------------------------------------------------------

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_APPLIC}  "Application ${WINBITS}-bit executable."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QT5}     "Qt 5 DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_MINGW}   "MingW DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTDLLS}  "Qt 5 DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTICON}  "Qt 5 icon-engine DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTIMG}   "Qt 5 image-format DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTPLAT}  "Qt 5 platform DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTSTYLE} "Qt 5 style DLLs."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_QTTRANS} "Qt 5 translation files."
!insertmacro MUI_DESCRIPTION_TEXT ${SEC_LIC}     "Licenses and sample files."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------------------------------------------------
; Uninstallation operations
;--------------------------------------------------------------------------

Function un.onUninstSuccess

  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."

FunctionEnd

Function un.onInit

  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort

FunctionEnd


; The Uninstall section is layed out as much as possible to match the
; sections listed above.
;
; Set up to call a few batch files

Section Uninstall

    ExpandEnvStrings $0 %COMSPEC%

;   Delete "$INSTDIR\license.text"
;   Delete "$INSTDIR\readme.text"

    Delete "$INSTDIR\uninst.exe"
    RMDir /r "$INSTDIR"

    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

SectionEnd

; vim: ts=4 sw=4 wm=3 et ft=nsis
