@echo off
:: **************************************************************************
:: Seq66 Windows Build-Release Package
:: --------------------------------------------------------------------------
::
:: \file        build_release_package.bat
:: \library     Seq66 for Windows
:: \author      Chris Ahlstrom
:: \date        2018-05-26
:: \update      2024-11-30
:: \license     $XPC_SUITE_GPL_LICENSE$
::
::      This script sets up and creates a release build of Seq66 for
::      Windows and creates a 7-Zip package that can be unpacked in the root
::      of the project, ready to be made into an NSIS installer either in Linux
::      or in Windows.
::
::      If NSIS is installed and the PATH leads to the makensis.exe program,
::      then a Seq66 NSIS installer will be created. Otherwise, that is a
::      separate manual step described below in steps 7 to 9.
::
:: Also see:
::
:: https://stackoverflow.com/questions/18553125/
::      how-qtcreator-is-able-to-avoid-the-console-window-when-building-a-windows-applic
::
:: Requirements:
::
::       0. Decide whether to a 32-bit or a 64-bit build.  Modify the
::          environment variables and steps below to accommodate your choice.
::          We might support only 64-bit builds at some point. And in fact,
::          32-bit and old OS's like Windows XP may not be supported.
::
::          https://forum.qt.io/topic/73292/
::              the-last-qt-version-that-supported-windows-xp/5
::
::          Warning: Currently a 32-bit build is not possible on our current
::          setup.
::
::       1. Runs in Windows only.
::       2. Requires QtCreator to be installed, and configured to provide
::          the 32/64-bit Mingw tools, including mingw32-make.exe (there is
::          no mingw64-make.exe, but the mingw810_32 and mingw810_64
::          directories both have a binary by the name), and qmake.exe. 
::          The PATH must include the path to both executables. See "Path
::          Additions" below.  We have not tried the Microsoft C++ compiler
::          yet. Any takers?
::       3. Requires 7-Zip to be installed and accessible from the DOS
::          command-line, as 7z.exe.
::
:: Path Additions:
::
::       QTVERSION=5.15.2
::
::       1. C:\Qt\Qt5.15.2\mingw81_32\bin           (or 64-bit)
::       2. C:\Qt\Tools\mingw810_32\bin             (ditto)
::       3. C:\Program Files (x86)\NSIS\Bin         (if NSIS installed on Win)
::       4. C:\Program Files\Git\mingw64\bin        (GNU commands like "xz")
::       5. C:\Program Files\Git\usr\bin            (has only git and shells)
::
::      Depending on the versions some things will be different.
::
::       a. For earlier versions of Qt, might need to remove "Qt" from the
::          "Qt5.12.9" subdirectories.
::       b. Might also need to change the "81" version number in "mingw81_32"
::          in the paths.
::       c. Can also change to 64-bit:  "mingw81_64".  In this case warnings or
::          errors might be exposed in the Windows PortMidi C files, though
::          we check both builds.
::
::          Note: the default is now PROJECT_BITS = 64. Support for 32-bits
::          in Qt does not seem to be viable anymore without a laborious
::          rebuild of Qt 5.12+ from source code.
::
:: Build Instructions:
::
::      Note that steps 6 through 10 can be performed on Linux with the
::      "packages" script.  On my Windows build machine, the source is placed
::      in C:\Users\chris\Documents\Home\seq66.
::
::       1. Before running this script, modify the environment variables below
::          in this batch file for your specific setup, including
::          PROJECT_VERSION, PROJECT_DRIVE, PROJECT_BITS, PROJECT_BASE,
::          and NSIS_PLATFORM (either "Linux" or "Windows").
::       2. Also edit seq66/nsis/Seq66Constants.nsh to specify the current
::          date and Seq66 version number.  The macros to modify are:
::          VER_NUMBER (e.g. "0.90") and VER_REVISION (e.g. "1", as in "0.90.1").
::       3. Run "./bootstrap --full-clean" to get rid of generated files.
::          Then run the "pack" script to create an xz tarball, such as
::          "seq66-master-2021-04-17-pack.tar.xz.
::       4. Copy this file to the desired directory on Windows, and use
::          a program like 7zip to extract it.  It takes two passes of
::          "7-Zip / Extract Here", one to extract the "xz" file, and one to
::          extract the "tar" file.
::       5. a. In Windows Explorer, make sure there is no existing Qt Creator
::             build or shadow directory/configuration, especially a Debug
::             configuration.
::          b. In Windows Explorer, just double-click on this batch file in its
::             location in the "nsis" directory, and watch the build run in a
::             DOS window.
::          c. Alternatively, create a "shadow" directory at the same level
::             as "seq66", change to it in a DOS console, and run
::             "..\seq66\nsis\build_release_package.bat".
::          d. Or cd to the seq66\nsis directory and try
::             "build_release_package.bat > build.log 2>&1"
::             This redirection of output is optional. A make.log is
::             created anyway.
::       6. The result is a file such as "qpseq66-release-package-x64-0.90.1.7z".
::          It is found in seq66/../seq66-release-32/Seq66qt5.  Also, a
::          log file is made in seq66/make.log,
::          which can be checked for build warnings and errors. If you cannot
::          find these files, search for 'seq66-release-32'.
::       7. In Linux, one can copy this 7z file to the root 'seq66' directory.
::          However, if NSIS is installed in Windows, the NSIS executable
::          package is built by the script in seq66/release. Otherwise,
::          If the directory 'seq66/release' exists, remove the 'release'
::          directory as shown in the next step.
::       8. Use 7zip to extract this file; it will unpack the contents into
::          the directory called 'release' ('seq66/release'), which contains
::          qpseq66.exe, DLL files, data, etc. Then move the 7z file out of
::          the way, for example to the directory about the seq66 directory.
::          Here are the commands ("$" is the command-line prompt character):
::
::          seq66 $ rm -rf release/                         # careful!
::          seq66 $ 7z x qpseq66-release-package-x64-0.90.2.7z
::          seq66 $ mv qpseq66-release-package-x64-0.90.2.7z ..
::
::       9. Change to the seq66/nsis directory and run:
::
::          seq66/nsis $ makensis Seq66Setup.nsi
::
::          However, if NSIS is installed in Windows, the NSIS executable
::          package is built by the script and resides in seq66/release.
::      10. The installer is seq66/release/seq66_setup_0.90.1.exe, and it is
::          in the 'release' directory.  Move it out of this directory to a
::          safe place for transport. For example, assuming the current
::          directory is 'release'.  One of these can be run:
::
::          seq66/release $ mv seq66_setup_0.90.1.exe \
::              ../../sequencer64-packages/seq66/0.90
::          seq66/release $ mv seq66_setup_0.90.1.exe\
::              ../../seq66/packages/... TO DO !!!
::
::          Note: setup files are now part of the GitHub releases of Seq66.
::      11. Make a portable Zip package:
::
::          $ mv release/ qpseq66
::          $ zip -u -r qpseq66-portable-0.90.1-0.zip qpseq66/
::          $ mv qpseq66-portable-0.90.1-0.zip ../seq66-packages/latest
::
::      13. Change to the 'seq66' directory to make a standard Linux
::          source/configure tarball for a version built using bootstrap
::          (to generate the "configure" script):
::
::          $ ./pack --release rtmidi 0.90.1
::          $ mv ../seq66-master-rtmidi-0.90.1.tar.xz \
::                  ../seq66-packages/latest
::
::          where "rtmidi" can be replaced with whatever the current build
::          is, such as "cli" or "portmidi" or "qt".
::
:: This batch file completely removes the old Windows seq66-release-64 or
:: 32 directory and re-does everything.
::
:: See the set of variable immediately below.
::
:: PROJECT_BASE is the directory that is the immediate parent of the seq66
:: directory.  Adjust this value for your setup.
::
:: Mingw:
::
:: set PROJECT_BASE=\home\chris\Home\git
::
:: NSIS_PLATFORM defaults to "Windows", but the presence of the makensis
:: program is tested; if missing, then the value is "Linux".
::
::---------------------------------------------------------------------------
 
set PROJECT_VERSION=0.99.16
set PROJECT_DATE=2024-11-30
set PROJECT_DRIVE=C:

:: Set the bits of the project, either 64 or 32. Also define WIN64 versus
:: WIN32 and set WINBItS to "32" in Seq66Constants.nsh.
::
:: set PROJECT_BITS=32      // probably no longer supportable by Qt
:: set PROJECT_BITS=64

set PROJECT_BITS=64
set QTVERSION=5.15.2
set MINGVERSION=mingw81
set TOOLVERSION=mingw810
set QTPATH=C:\Qt\%QTVERSION%
set QMAKE=%QTPATH%\%MINGVERSION%_%PROJECT_BITS%\bin\qmake.exe
set QMAKEOPTS=-makefile -recursive

:: Old:
:: set MAKEPATH=C:\Qt\Qt%QTVERSION%\Tools\%TOOLVERSION%_%PROJECT_BITS%\bin

set MAKEPATH=C:\Qt\Tools\%TOOLVERSION%_%PROJECT_BITS%\bin
set MINGMAKE=%MAKEPATH%\mingw32-make

:: This is where the seq66 and the shadow build directories reside. Adjust
:: them for your setup.

set PROJECT_BASE=\Users\%USERNAME%\Documents\git
set NSIS_PLATFORM=Windows

:: PROJECT_REL_ROOT is relative to the Qt shadow build directory created
:: in PROJECT_BASE.

set PROJECT_NAME=seq66
set PROJECT_TREE=%PROJECT_DRIVE%%PROJECT_BASE%\%PROJECT_NAME%
set PROJECT_REL_ROOT=..\seq66
set PROJECT_PRO=seq66.pro
set PROJECT_7ZIP=qpseq66-release-package-x%PROJECT_BITS%-%PROJECT_VERSION%.7z
set SHADOW_DIR=..\seq66-release-%PROJECT_BITS%
set APP_DIR=Seq66qt5
set LOG=..\seq66\make.log
set RELEASE_DIR=%APP_DIR%\release
set AUX_DIR=data
set DOC_DIR=data\share\doc
set TUTORIAL_DIR=data\share\doc\tutorial
set INFO_DIR=data\share\doc\info

:: The quotes are required here. Do we want to add "qtquickcompiler"?

set CONFIG_SET="CONFIG += release WIN%PROJECT_BITS%"

:: Set the current drive (normally C:). Then change to the user's git
:: directory + seq66. We now put the make.log file there instead of the
:: shadow directory.

%PROJECT_DRIVE%
cd %PROJECT_BASE%\%PROJECT_NAME%
echo Starting the build for this project directory: > make.log 2>&1
cd >> make.log 2>&1
del /S /Q %SHADOW_DIR%\*.* > NUL
echo Recreating Qt shadow directory %SHADOW_DIR% ... >> make.log 2>&1
rmdir %SHADOW_DIR%
mkdir %SHADOW_DIR%

:: Make sure the supplementary batch files are in the shadow directory.

cd %SHADOW_DIR%
set > environment.log
cd >> %LOG% 2>&1

:: TODO: add "-spec win32-g++" for 32 bit and ??? for 64-bit

echo %QMAKE% %QMAKEOPTS% %CONFIG_SET% %PROJECT_REL_ROOT%\%PROJECT_PRO% >> %LOG% 2>&1
%QMAKE% %QMAKEOPTS% %CONFIG_SET% %PROJECT_REL_ROOT%\%PROJECT_PRO% >> %LOG% 2>&1
echo %MINGMAKE% with output to make.log >> %LOG% 2>&1
%MINGMAKE% >> %LOG% 2>&1

:: if %ERRORLEVEL% NEQ 0 goto builderror

if ERRORLEVEL 1 goto builderror

echo Compiling and linking succeeded >> %LOG% 2>&1

if %PROJECT_BITS% == 64 goto windep64
echo Running brute-force windeploy ... >> %LOG% 2>&1
call ..\seq66\nsis\winddeploybruteforce %QTVERSION% %MINGVERSION%_32 %RELEASEDIR%
goto makerels

:: windeployqt Seq66qt5\release

:windep64

echo Creating deployment area via windeployqt %RELEASE_DIR% >> %LOG% 2>&1
windeployqt %RELEASE_DIR%

:makerels

echo Creating data directories in %RELEASE_DIR% >> %LOG% 2>&1
echo mkdir %RELEASE_DIR%\%AUX_DIR%
echo mkdir %RELEASE_DIR%\%DOC_DIR%
echo mkdir %RELEASE_DIR%\%TUTORIAL_DIR%
echo mkdir %RELEASE_DIR%\%INFO_DIR%
echo copy %PROJECT_REL_ROOT%\%DOC_DIR%\*.pdf %RELEASE_DIR%\%DOC_DIR%
echo copy %PROJECT_REL_ROOT%\%DOC_DIR%\*.ods %RELEASE_DIR%\%DOC_DIR%
echo xcopy %PROJECT_REL_ROOT%\%DOC_DIR%\tutorial\*.* %RELEASE_DIR%\%TUTORIAL_DIR% /f /s /e /y /i
echo xcopy %PROJECT_REL_ROOT%\%DOC_DIR%\info\*.* %RELEASE_DIR%\%INFO_DIR% /f /s /e /y /i

mkdir %RELEASE_DIR%\%AUX_DIR%
mkdir %RELEASE_DIR%\%AUX_DIR%\linux
mkdir %RELEASE_DIR%\%AUX_DIR%\midi
mkdir %RELEASE_DIR%\%AUX_DIR%\pixmaps
mkdir %RELEASE_DIR%\%AUX_DIR%\samples
mkdir %RELEASE_DIR%\%AUX_DIR%\win
mkdir %RELEASE_DIR%\%AUX_DIR%\wrk
mkdir %RELEASE_DIR%\%DOC_DIR%
mkdir %RELEASE_DIR%\%TUTORIAL_DIR%
mkdir %RELEASE_DIR%\%INFO_DIR%

copy %PROJECT_REL_ROOT%\%AUX_DIR%\license.* %RELEASE_DIR%\%AUX_DIR%
copy %PROJECT_REL_ROOT%\%AUX_DIR%\readme.* %RELEASE_DIR%\%AUX_DIR%
copy %PROJECT_REL_ROOT%\%AUX_DIR%\linux\*.* %RELEASE_DIR%\%AUX_DIR%\linux
xcopy %PROJECT_REL_ROOT%\%AUX_DIR%\midi\*.* %RELEASE_DIR%\%AUX_DIR%\midi /f /s /e /y /i
copy %PROJECT_REL_ROOT%\%AUX_DIR%\pixmaps\*.* %RELEASE_DIR%\%AUX_DIR%\pixmaps
copy %PROJECT_REL_ROOT%\%AUX_DIR%\samples\*.* %RELEASE_DIR%\%AUX_DIR%\samples
copy %PROJECT_REL_ROOT%\%AUX_DIR%\win\*.* %RELEASE_DIR%\%AUX_DIR%\win
copy %PROJECT_REL_ROOT%\%AUX_DIR%\wrk\*.* %RELEASE_DIR%\%AUX_DIR%\wrk
copy %PROJECT_REL_ROOT%\%DOC_DIR%\README %RELEASE_DIR%\%DOC_DIR%
copy %PROJECT_REL_ROOT%\%DOC_DIR%\*.pdf %RELEASE_DIR%\%DOC_DIR%
copy %PROJECT_REL_ROOT%\%DOC_DIR%\*.ods %RELEASE_DIR%\%DOC_DIR%
xcopy %PROJECT_REL_ROOT%\%DOC_DIR%\tutorial\*.* %RELEASE_DIR%\%TUTORIAL_DIR% /f /s /e /y /i
xcopy %PROJECT_REL_ROOT%\%DOC_DIR%\info\*.* %RELEASE_DIR%\%INFO_DIR% /f /s /e /y /i

:: This section takes the generated build and data files and packs them
:: up into a 7-zip archive.  This archive should be copied to the root
:: directory (seq66) and extracted (the contents go into the release
:: directory.
::
:: Then, in Linux, "cd" to the "nsis" directory and run
::
::      makensis Seq66Setup.nsi
::
:: pushd Seq66qt5
:: 7z a -r qppseq66-nsis-ready-package-DATE.7z release\*

echo The build DLLs and EXE are in %SHADOW_DIR%\%RELEASE_DIR% >> %LOG% 2>&1
pushd %APP_DIR%
echo Making 7zip package in %APP_DIR%... >> %LOG% 2>&1
cd
echo 7z a -r %PROJECT_7ZIP% release\* >> %LOG% 2>&1
del *.o
7z a -r %PROJECT_7ZIP% release\*
popd

:: Test for a properly set up NSIS on Windows.

echo Testing for Windows NSIS/makensis ... >> %LOG% 2>&1
makensis /version
if ERRORLEVEL 0 goto nsisexists

echo NSIS "makensis" for Windows not found, skipping it... >> %LOG% 2>&1
set NSIS_PLATFORM=Linux

:: Here we are seq66-release-64 (or 32).

:nsisexists

if NOT %NSIS_PLATFORM% == Windows goto skipnsis

echo Copying the 7zip package to the project tree... >> %LOG% 2>&1
del /S /Q %PROJECT_TREE%\release\*.* > NUL
copy %APP_DIR%\%PROJECT_7ZIP% %PROJECT_TREE%
cd %PROJECT_TREE%
echo Unpacking the 7zip package, %PROJECT_7ZIP% ... >> %LOG% 2>&1
7z x %PROJECT_7ZIP%
echo Building a Windows installer using NSIS... >> %LOG% 2>&1
cd %PROJECT_TREE%
pushd nsis
makensis Seq66Setup.nsi
echo If makensis succeeded, the installer is located in >> %LOG% 2>&1
echo %PROJECT_TREE%\release, named like "seq66_setup_VERSION.exe". >> %LOG% 2>&1
popd
goto done

:builderror

echo %MINGMAKE% failed, aborting! Check %LOG% for errors. >> %LOG% 2>&1
goto ender

:skipnsis

echo The NSIS installer builder is not installed on this Window computer. >> %LOG% 2>&1
echo In Linux, copy the %PROJECT_7ZIP% file to the project root ("seq66") >> %LOG% 2>&1
echo and extract that file. The contents go into the "release" directory. >> %LOG% 2>&1
echo Change to the "nsis" directory and run "makensis Seq66Setup.nsi". >> %LOG% 2>&1

:done

echo Build products are in %RELEASE_DIR%. >> %LOG% 2>&1

:ender

:: vim: ts=4 sw=4 ft=dosbatch fileformat=dos
