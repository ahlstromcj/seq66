@echo off

:: **************************************************************************
:: Build Release Package
:: --------------------------------------------------------------------------
::
:: \file        build_release_package.bat
:: \library     Seq66 for Windows
:: \author      Chris Ahlstrom
:: \date        2018-05-26
:: \update      2019-09-22
:: \license     $XPC_SUITE_GPL_LICENSE$
::
::      This script sets up and creates a release build of Seq66 for
::      Windows and creates a 7-Zip package that can be unpacked in the root
::      of the project, ready to made into an NSIS installer either in Linux
::      or in Windows.
::
:: Requirements:
::
::       1. Runs in Windows only.
::       2. Requires QtCreator to be installed, and configured to provide
::          the 32-bit Mingw tools, including mingw32-make.exe, and
::          qmake.exe.  The PATH must included the path to both executables.
::          We have not tried using the Microsoft C++ compiler yet.
::       3. Requires 7-Zip to be installed and accessible from the DOS
::          command-line, as 7z.exe.
::
:: Instructions:
::
::      Note that steps 5 through 9 can be performed on Linux with the
::      "packages" script.
::
::       1. Before running this script, modify the environment variables below
::          in this batch file for your specific setup, including
::          PROJECT_VERSION, PROJECT_DRIVE, and PROJECT_BASE.
::       2. Also edit seq66/nsis/Seq66Constants.nsh
::          to specify the current date and Seq66 version number.
::          The macros to modify are: VER_NUMBER (e.g. "0.95") and
::          VER_REVISION (e.g. "1", as in "0.95.1").
::       3. a. In Windows Explorer, make sure there is no existing Qt Creator
::             build or shadow directory/configuration, especially a Debug
::             configuration.
::          b. In Windows Explorer, just double-click on this batch file in its
::             location in the "nsis" directory, and watch the build run in a
::             DOS window.
::          c. Alternatively, create a "shadow" directory at the same level
::             as "seq66", change to it, and run
::             "..\seq66\nsis\build_release_package.bat".
::       4. The result is a file such as "qpseq66-release-package-0.95.1.7z".
::          It is found in ../seq66/seq66-release/Seq66qt5.  Also, a
::          log file is made in ../seq66/seq66-release/make.log,
::          which can be checked for build warnings and errors. If you cannot
::          find these files, search for "seq66-release".
::       5. In Linux (have not tried NSIS in Windows yet), copy this 7z file
::          to the root seq66 directory.
::       6. Use 7zip to extract this file; it will unpack the contents into
::          the 'release' directory.  Then move the 7z file out of the way.
::
::          $ 7z x qpseq66-release-package-0.95.2.7z
::          $ mv qpseq66-release-package-0.95.2.7z ..
::
::       7. Change to the seq66/nsis directory and run:
::
::          $ makensis Seq66Setup.nsi
::
::       8. The installer is seq66/release/seq66_setup_0.95.1.exe.
::          Move it out of this directory to a safe place for transport.
::          For example:
::
::          $ mv seq66_setup_0.95.1.exe ../../seq66-packages/latest
::
::       9. Make a portable Zip package:
::
::          $ mv release/ qpseq66
::          $ zip -u -r qpseq66-portable-0.95.1-0.zip qpseq66/
::          $ mv qpseq66-portable-0.95.1-0.zip ../seq66-packages/latest
::
::      10. Make a standard Linux source/configure tarball for a version
::          built using bootstrap (to generate the "configure" script):
::
::          $ ./pack --release rtmidi 0.95.1
::          $ mv ../seq66-master-rtmidi-0.95.1.tar.xz \
::                  ../seq66-packages/latest
::
::          where "rtmidi" can be replaced with whatever the current build
::          is, such as "cli" or "portmidi" or "qt".
::
:: This batch file completely removes the old Windows seq66-release directory
:: and re-does everything.
::
:: See the set of variable immediately below.
::
::---------------------------------------------------------------------------
 
set PROJECT_VERSION=0.90.1
set PROJECT_DRIVE=C:
set PROJECT_BASE=\Users\Chris\Documents\Home
set PROJECT_ROOT=..\seq66
set PROJECT_FILE=seq66.pro
set PROJECT_7ZIP="qpseq66-release-package-%PROJECT_VERSION%.7z"
set SHADOW_DIR=seq66-release
set APP_DIR=Seq66qt5
set OUTPUT_DIR=%APP_DIR%\release
set CONFIG_SET="CONFIG += release"
set AUX_DIR=data

:: C:

%PROJECT_DRIVE%

:: cd \Users\Chris\Documents\Home

cd %PROJECT_BASE%

:: mkdir seq66-release
:: cd seq66-release

del /S /Q %SHADOW_DIR% > NUL
mkdir %SHADOW_DIR%
cd %SHADOW_DIR%

:: qmake -makefile -recursive "CONFIG += release" ..\seq66\seq66.pro

cd
echo qmake -makefile -recursive %CONFIG_SET% %PROJECT_ROOT%\%PROJECT_FILE%
echo mingw32-make (output to make.log)
qmake -makefile -recursive %CONFIG_SET% %PROJECT_ROOT%\%PROJECT_FILE%
mingw32-make > make.log 2>&1

:: windeployqt Seq66qt5\release

echo windeployqt %OUTPUT_DIR%
windeployqt %OUTPUT_DIR%

:: mkdir Seq66qt5\release\data
:: copy ..\seq66\data\*.midi Seq66qt5\release\data
:: copy ..\seq66\data\qpseq66.* Seq66qt5\release\data
:: copy ..\seq66\data\*.pdf Seq66qt5\release\data
:: copy ..\seq66\data\*.txt Seq66qt5\release\data

echo mkdir %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\qpseq66.* %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\*.midi %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\*.pdf %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\*.txt %OUTPUT_DIR%\%AUX_DIR%
echo copy %PROJECT_ROOT%\%AUX_DIR%\*.playlist %OUTPUT_DIR%\%AUX_DIR%

mkdir %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.rc %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.usr %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.midi %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.pdf %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.txt %OUTPUT_DIR%\%AUX_DIR%
copy %PROJECT_ROOT%\%AUX_DIR%\*.playlist %OUTPUT_DIR%\%AUX_DIR%

:: This section takes the generated build and data files and packs them
:: up into a 7-zip archive.  This archive should be copied to the root
:: directory (seq66) and extracted (the contents go into the release
:: directory.
::
:: Then, in Linux, "cd" to the "nsis" directory and run
::
::      makensis Seq66Setup_V0.95.nsi
::
:: pushd Seq66qt5
:: 7z a -r qppseq66-nsis-ready-package-DATE.7z release\*

pushd %APP_DIR%
cd
echo 7z a -r %PROJECT_7ZIP% release\*
7z a -r %PROJECT_7ZIP% release\*
popd

:: vim: ts=4 sw=4 ft=dosbatch fileformat=dos
