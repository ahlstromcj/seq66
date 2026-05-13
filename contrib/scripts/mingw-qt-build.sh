#!/bin/sh
#
#******************************************************************************
# mingw-qt-build.sh (seq66)
#------------------------------------------------------------------------------
##
# \file           mingw-qt-build.sh
# \library        seq66
# \author         Chris Ahlstrom
# \date           2026-05-13
# \update         2026-05-13
# \version        $Revision$
# \license        $XPC_SUITE_GPL_LICENSE$
#
#     The above is modified by the following to remove even the mild GPL
#     restrictions:
#
#     Use this script in any manner whatsoever.  You don't even need to give
#     me any credit.  However, keep in mind the value of the GPL in keeping
#     software and its descendant modifications available to the community for
#     all time.
#
#     This script is meant to install the Windows version of the basic
#     Qt6
#
#     https://aqtinstall.readthedocs.io/en/latest/getting_started.html
#
#     See contrib/notes/mingw-qt-build.text.
#
#     The defaults defined below result in following directories:
#
#        -  /opt/Qt/Qt6/6.10.2/mingw_64/bin
#           -  moc.exe; rcc.exe; uic.exe; qmake[6].exe;
#              windeployqt[6].exe; etc.
#           -  Qt6Core5Compat.dll; Qt6Core.dll; Qt6Gui.dll;
#              Qt6Widgets.dll; etc.
#           -  Can add this path to PATH
#
#        -  /opt/Qt/Qt6/6.10.2/mingw_64/lib
#           -  libQt6Core.a and the rest of the .a files
#           -  Qt6Core.prl and the rest of the .prl files
#           -  Can add this path to LIBRARY_PATH or LD_LIBRARY_PATH.
#           -  Can set a variable to this path and declare a dependency
#              with link_args : [ '-L' + lib_dir, '-l' + lib_name ]
#
#        -  /opt/Qt/Qt6/6.10.2/mingw_64/lib/pkgconfig
#           -  Qt6Core.pc and the rest of the .pc files
#           -  Can add this path to PKG_CONFIG_PATH
#           -  Can add this path to meson setup via --pkg-config-path
#
#        -  /opt/Qt/Qt6/6.10.2/mingw_64/include
#           -  The <QtXyx> headers.
#           -  The q*.h headers.
#           -  Can assign this directory to a varialb ein meson.build
#              and add it to include_directories()
#
#        -  /opt/Qt/Qt6/6.10.2/mingw_64/<others>
#           -  metatypes
#           -  mkspecs
#           -  modules
#           -  etc.
#
#------------------------------------------------------------------------------

LANG=C
export LANG
CYGWIN=binmode
export CYGWIN
export MINGW_SCRIPT_EDIT_DATE="2026-05-13"
OUTDIR="/opt/Qt/Qt6"
MINGHOST="windows"
MINGTARGET="desktop"
ARCHITECTURE="win64_mingw"
QTVERSION="6.10.2"
QTMODULES="qt5compat"
QTARCHIVES="qtbase"
DRYRUN="" # DRYRUN="--dry-run"

mkdir -p $OUTDIR
if test $? = 0 ; then

   # Downloads the Qt DLLs and headers compiled for Windows

   echo "Run 'aqt install-qt $DRYRUN --outputdir $OUTDIR $MINGHOST $MINGTARGET"
   echo "       $QTVERSION $ARCHITECTURE -m $QTMODULES --archives $QTARCHIVES'"

   aqt install-qt $DRYRUN --outputdir $OUTDIR $MINGHOST $MINGTARGET \
      $QTVERSION $ARCHITECTURE -m $QTMODULES --archives $QTARCHIVES
else
    echo "Failed to make $OUTDIR"
fi

#******************************************************************************
# mingw-qt-build.sh (seq66)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 wm=4 et ft=sh
#------------------------------------------------------------------------------
