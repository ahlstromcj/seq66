@echo off
:: **************************************************************************
:: Seq66 Windows Build-Release Package
:: --------------------------------------------------------------------------
::
:: \file        build_release_package.bat
:: \library     Seq66 for Windows
:: \author      Chris Ahlstrom
:: \date        2018-05-26
:: \update      2023-05-09
:: \license     $XPC_SUITE_GPL_LICENSE$
::
::      This script sets up and creates a release build of Seq66 for
::      Windows and creates a 7-Zip package that can be unpacked in the root
::      of the project, ready to be made into an NSIS installer either in Linux
::      or in Windows.  It does NOT make the NSIS installer; that is a separate
::      manual step described below in steps 5 to 8.
::
:: Requirements:
::
::       0. Decide whether to a 32-bit or a 64-bit build.  Modify the
::          environment variables and steps below to accommodate your choice.
::       1. Runs in Windows only.
::       2. Requires QtCreator to be installed, and configured to provide
::          the 32/64-bit Mingw tools, including mingw32-make.exe (there is
::          no mingw64-make.exe), and qmake.exe.  The PATH must include the
::          path to both executables. See "Path Additions" below.
::          We have not tried the Microsoft C++ compiler yet. Any takers?
::       3. Requires 7-Zip to be installed and accessible from the DOS
::          command-line, as 7z.exe.
::
:: Path Additions:
::
::       1. C:\Qt\Qt5.12.9\5.12.9\mingw73_32\bin    (or 64-bit)
::       2. C:\Qt\Qt5.12.9\Tools\mingw73_32\bin     (ditto)
::       3. C:\Program Files (x86)\NSIS\bin         (if NSIS installed on Win)
::       4. C:\Program File\Git\usr\bin etc.        (if Git Bash installed)
::
::      Depending on the versions some things will be different.
::
::       a. For earlier versions of Qt, might need to remove "Qt" from the
::          "Qt5.12.9" subdirectories.
::       b. Might also need to change the "73" version number in "mingw73_32"
::          in the paths.
::       c. Can also change to 64-bit:  "mingw73_64".  In this case warnings or
::          errors might be exposed in the Windows PortMidi C files, though
::          we check both builds.
::
::          Note: the default is now PROJECT_BITS = 64.
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
::       6. The result is a file such as "qpseq66-release-package-0.90.1.7z".
::          It is found in seq66/../seq66-release-32/Seq66qt5.  Also, a
::          log file is made in seq66/../seq66-release-32/make.log,
::          which can be checked for build warnings and errors. If you cannot
::          find these files, search for 'seq66-release-32'.
::       7. In Linux (have not tried NSIS in Windows yet), copy this 7z file
::          to the root 'seq66' directory.  If the directory 'seq66/release'
::          exists, remove the 'release' directory as shown in the next step.
::       8. Use 7zip to extract this file; it will unpack the contents into
::          the directory called 'release' ('seq66/release'), which contains
::          qpseq66.exe, DLL files, data, etc. Then move the 7z file out of
::          the way, for example to the directory about the seq66 directory.
::          Here are the commands ("$" is the command-line prompt character):
::
::          seq66 $ rm -rf release/                         # careful!
::          seq66 $ 7z x qpseq66-release-package-0.90.2.7z
::          seq66 $ mv qpseq66-release-package-0.90.2.7z ..
::
::       9. Change to the seq66/nsis directory and run:
::
::          seq66/nsis $ makensis Seq66Setup.nsi
::
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
 
set PROJECT_VERSION=0.99.5
set PROJECT_DRIVE=C:
set PROJECT_BITS=64
set PROJECT_BASE=\Users\Chris\Documents\Home
set NSIS_PLATFORM=Windows

:: No need to change the following. PROJECT_REL_ROOT is relative to the
:: Qt shadow build directory that is created in PROJECT_BASE.

set PROJECT_NAME=seq66
set PROJECT_TREE=%PROJECT_BASE%\%PROJECT_NAME%
set PROJECT_REL_ROOT=..\seq66
set PROJECT_FILE=seq66.pro
set PROJECT_7ZIP="qpseq66-release-package-%PROJECT_VERSION%.7z"
set SHADOW_DIR=seq66-release-%PROJECT_BITS%
set APP_DIR=Seq66qt5
set RELEASE_DIR=%APP_DIR%\release
set CONFIG_SET="CONFIG += release"
set AUX_DIR=data
set DOC_DIR="data\share\doc"

:: C:

%PROJECT_DRIVE%

:: cd \Users\Chris\Documents\Home

cd %PROJECT_BASE%
del /S /Q %SHADOW_DIR% > NUL
mkdir %SHADOW_DIR%
echo Creating Qt shadow directory %SHADOW_DIR% ...
cd %SHADOW_DIR%

:: qmake -makefile -recursive "CONFIG += release" ..\seq66\seq66.pro

cd
echo qmake -makefile -recursive %CONFIG_SET% %PROJECT_REL_ROOT%\%PROJECT_FILE%
echo mingw32-make (output to make.log)
qmake -makefile -recursive %CONFIG_SET% %PROJECT_REL_ROOT%\%PROJECT_FILE% > make.log 2>&1
mingw32-make > make.log 2>&1

:: windeployqt Seq66qt5\release

echo Creating deployment area via windeployqt %RELEASE_DIR%
windeployqt %RELEASE_DIR%

echo mkdir %RELEASE_DIR%\%AUX_DIR%
echo mkdir %RELEASE_DIR%\%DOC_DIR%
echo copy %PROJECT_REL_ROOT%\%DOC_DIR%\*.pdf %RELEASE_DIR%\%DOC_DIR%
echo copy %PROJECT_REL_ROOT%\%DOC_DIR%\*.ods %RELEASE_DIR%\%DOC_DIR%
echo xcopy %PROJECT_REL_ROOT%\%DOC_DIR%\tutorial %RELEASE_DIR%\%DOC_DIR% /f /s /y /i

mkdir %RELEASE_DIR%\%AUX_DIR%
mkdir %RELEASE_DIR%\%AUX_DIR%\linux
mkdir %RELEASE_DIR%\%AUX_DIR%\midi
mkdir %RELEASE_DIR%\%AUX_DIR%\samples
mkdir %RELEASE_DIR%\%AUX_DIR%\win
mkdir %RELEASE_DIR%\%AUX_DIR%\wrk

copy %PROJECT_REL_ROOT%\%AUX_DIR%\linux\*.* %RELEASE_DIR%\%AUX_DIR%\linux
copy %PROJECT_REL_ROOT%\%AUX_DIR%\midi\*.* %RELEASE_DIR%\%AUX_DIR%\midi
copy %PROJECT_REL_ROOT%\%AUX_DIR%\samples\*.* %RELEASE_DIR%\%AUX_DIR%\samples
copy %PROJECT_REL_ROOT%\%AUX_DIR%\win\*.* %RELEASE_DIR%\%AUX_DIR%\win
copy %PROJECT_REL_ROOT%\%AUX_DIR%\wrk\*.* %RELEASE_DIR%\%AUX_DIR%\wrk

mkdir %RELEASE_DIR%\%DOC_DIR%
copy %PROJECT_REL_ROOT%\%DOC_DIR%\*.pdf %RELEASE_DIR%\%DOC_DIR%
copy %PROJECT_REL_ROOT%\%DOC_DIR%\*.ods %RELEASE_DIR%\%DOC_DIR%
copy %PROJECT_REL_ROOT%\%DOC_DIR%\README %RELEASE_DIR%\%DOC_DIR%

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

echo The build DLLs and EXE are in %SHADOW_DIR%\%RELEASE_DIR%
pushd %APP_DIR%
echo Making 7zip package in %APP_DIR%...
cd
echo 7z a -r %PROJECT_7ZIP% release\*
del *.o
7z a -r %PROJECT_7ZIP% release\*
popd

:: Test for a properly set up NSIS on Windows.

makensis /version
if ERRORLEVEL 0 goto nsisexists

set NSIS_PLATFORM=Linux

:: Here we are seq66-release-64 (or 32).

:nsisexists

if NOT %NSIS_PLATFORM%==Windows goto skipnsis

echo "Copying the 7zip package to the project tree..."
copy %APP_DIR%\%PROJECT_7ZIP% %PROJECT_TREE%
cd %PROJECT_TREE%
echo "Unpacking the 7zip package, %PROJECT_7ZIP% ..."
7z x %PROJECT_7ZIP%
echo "Building a Windows installer using NSIS..."
cd %PROJECT_TREE%
pushd nsis
makensis Seq66Setup.nsi
echo If makensis succeeded, the installer is located in
echo %PROJECT_TREE%\release, named like "seq66_setup_VERSION.exe".
popd
goto done

:skipnsis

echo In Linux, copy the %PROJECT_7ZIP% file to the project root ("seq66")
echo and extract that file. The contents go into the "release" directory.
echo Change to the "nsis" directory and run "makensis Seq66Setup.nsi".

:done

echo Build products are in %RELEASE_DIR%.

:: vim: ts=4 sw=4 ft=dosbatch fileformat=dos
