@echo off
:: **************************************************************************
:: Seq66 Windows Build-Debug Package
:: --------------------------------------------------------------------------
::
:: \file        build_debug_code.bat
:: \library     Seq66 for Windows
:: \author      Chris Ahlstrom
:: \date        2021-12-09
:: \update      2023-05-08
:: \license     $XPC_SUITE_GPL_LICENSE$
::
::      This script sets up and creates a debug build of Seq66 for
::      Windows.  We needed it to figure out why Seq66 uses less CPU when
::      playing than when not playing, and uses a high amount of CPU when
::      not playing.  And using Qt Creator's debugger doesn't cut it.
::      We won't repeat any discussion found in the build_release_package
::      batch file.
::
:: Requirements:
::
::       1. A working build setup as specified in the build_release_package
::          script, minus 7-zip.
::       2. Make sure Git Bash is installed.  The DOS command line does not
::          work well with gdb.
::
:: Build Instructions:
::
::       1. Before running this script, modify the environment variables below
::          in this batch file for your specific setup, including
::          PROJECT_VERSION, PROJECT_DRIVE, and PROJECT_BASE.
::       2. Create a "shadow" directory at the same level as "seq66", change
::          to it in a DOS console, and run
::             "..\seq66\nsis\build_debug_code.bat".
::          d. Or cd to the seq66\nsis directory and try
::             "build_release_package.bat > build.log 2>&1"
::       6. The result is.....
::          It is found in seq66/../seq66-debug/Seq66qt5.  A log file is
::          made in seq66/../seq66-debug/make.log, which can be checked
::          for build warnings and errors.
::
:: This batch file completely removes the old Windows seq66-release directory
:: and re-does everything.
::
:: Debug Instructions:
::
::      1. In a Git Bash window, change to the seq66-debug/Seq66qt5/debug
::         directory.
::      2. Run "gdb qpseq66.exe".
::      3. When desired, type Ctrl-C to pause the application.
::      4. Show the threads with "info threads".
::      5. Got to the desired thread using "thread 8" (for example).
::
::      x. After making a change to a source-code file:
::         a. Delete the existing qpseq66.exe; the make process somehow
::            won't rebuild it if it exists!
::         b. Go to seq66-debug and run "mingw32-make" (again).
::         c. Now the new version of qpseq66.exe.
::
::
:: See the set of variable immediately below.
::
::---------------------------------------------------------------------------
 
set PROJECT_VERSION=0.99.5
set PROJECT_DRIVE=C:
set PROJECT_BITS=32
set PROJECT_BASE=\Users\Chris\Documents\Home

:: The project directory depends upon running this file from the nsis
:: directory or from the shadow directory at the same level as "seq66".

set PROJECT_ROOT=..\seq66
set PROJECT_FILE=seq66.pro
set SHADOW_DIR=seq66-debug
set APP_DIR=Seq66qt5
set DEBUG_DIR=%APP_DIR%\debug
set CONFIG_SET="CONFIG += debug"
set AUX_DIR=data
set DOC_DIR=doc

:: C:

%PROJECT_DRIVE%

:: cd \Users\Chris\Documents\Home

cd %PROJECT_BASE%

:: mkdir seq66-debug
:: cd seq66-debug

del /S /Q %SHADOW_DIR% > NUL
mkdir %SHADOW_DIR%
cd %SHADOW_DIR%

:: qmake -makefile -recursive "CONFIG += debug" ..\seq66\seq66.pro

echo qmake -makefile -recursive %CONFIG_SET% %PROJECT_ROOT%\%PROJECT_FILE%
echo mingw%PROJECT_BITS%-make (output to make.log)
qmake -makefile -recursive %CONFIG_SET% %PROJECT_ROOT%\%PROJECT_FILE%
mingw%PROJECT_BITS%-make > make.log 2>&1

:: windeployqt Seq66qt5\debug

echo windeployqt %DEBUG_DIR%
windeployqt %DEBUG_DIR%

:: vim: ts=4 sw=4 ft=dosbatch fileformat=dos
