#!/bin/sh
#------------------------------------------------------------------------------ 
# \file        qbuildwin.sh
# \library     bin
# \author      Chris Ahlstrom
# \date        2023-12-23 to 2024-10-19
# \version     $Revision$ 
# \license     GNU GPLv2 or above
#
# This script runs qmake and make, and should be run from a Qt shadow
# directory.  Look at the variables below and make any changes needed for
# your setup.
#
# This version is preset for Windows builds in Msys2.
#
# Also see:
#
#  https://www.kienlab.com/posts/windows-qt5-deploy-static-linking-msys2/
#
#------------------------------------------------------------------------------

BUILDTYPE="release"
ENGINE="portmidi"
DOBUILD="yes"
QMAKER="qmake"
MAKER="make"
CPUCOUNT="4"
MAKEOPTIONS="-j 4"
MAKELOG="make.log"
QOPTIONS="-makefile -recursive -Wall"
PROJFILE="../seq66/seq66.pro"
PROJFILE2="seq66.pro"

#******************************************************************************
#  Help
#------------------------------------------------------------------------------

if test "$1" = "--help" -o "$1" = "help" ; then

   cat << E_O_F
This script makes it easy to build Qt shadow builds for Seq66, to generate
either qpseq66 (portmidi) or qseq66 (rtmidi). The default is to build 'qseq66'
as a release uisng the rtmidi engine. The build can be modified by adding the
flags 'debug' or 'portmidi' (which generates 'qpseq66').

    cd ..                        (parent directory of seq66)
    mkdir shadow-rtmidi          (if not already made)
    cd shadow-rtmidi
    rm -rf *                     (removed old junk; CAREFUL!)
    qbuild.sh                    (could add 'debug rtmidi', etc.)
    vi make.log

This script must be run from a shadow directory. The build output is written
to make.log.  Other options are '--help' and '--dry-run'.  Note that, if any
Qt 'pro' file in the project has changed, one should first remove all files
from the shadow directory (CAUTION!!!) before rebuilding. Options:

   --help         Show this help text.
   --cpus n       Use n cores. The default is 8.
   --dry-run      Do not build, just show the steps
   --debug        Enable a debug build.
   --portmidi     Build using Seq66's internal portmidi engine.
   --rtmidi       Build using Seq66's internal rtmidi engine [default].

E_O_F
   exit 1
fi

if test $# -ge 1 ; then

   while [ "$1" != "" ] ; do

      case "$1" in

         cpus | --cpus)
            shift
            CPUCOUNT=="$1"
            MAKEOPTIONS="-j $CPUCOUNT"
            ;;

         dry | --dry-run)
            DOBUILD="no"
            ;;

         debug | --debug)
            BUILDTYPE="debug"
            ;;

         portmidi | --portmidi)
            ENGINE="portmidi"
            ;;

         rtmidi | rt | --rtmidi)
            ENGINE="rtmidi"
            ;;

         *)
            echo "? Unsupported qbuild option; --help for more information"
            exit 1
            ;;

      esac
      shift
   done

fi

if test "$DOBUILD" = "yes" ; then
   rm -f $MAKELOG
   date > $MAKELOG
   echo " " >> $MAKELOG 2>&1
   echo "  $QMAKER $QOPTIONS \""CONFIG += $BUILDTYPE $ENGINE\"" $PROJFILE" >> $MAKELOG 2>&1
   echo " " >> $MAKELOG
   echo "  $QMAKER $QOPTIONS \""CONFIG += $BUILDTYPE $ENGINE\"" $PROJFILE2..."
   $QMAKER $QOPTIONS "CONFIG += $BUILDTYPE $ENGINE" $PROJFILE >> $MAKELOG 2>&1
   echo " " >> $MAKELOG
   echo "  $MAKER $MAKEOPTIONS > $MAKELOG" >> $MAKELOG 2>&1
   echo " " >> $MAKELOG
   echo "  $MAKER $MAKEOPTIONS > $MAKELOG ..."
   $MAKER $MAKEOPTIONS >> $MAKELOG 2>&1
   echo "Read $MAKELOG for build errors or warnings."
else
   echo "Commands to be run:"
   echo "  rm -f $MAKELOG"
   echo "  $QMAKER $QOPTIONS \""CONFIG += $BUILDTYPE $ENGINE\"" $PROJFILE"
   echo "  $MAKER $MAKEOPTIONS > $MAKELOG"
   echo "  $MAKELOG will contain the build details."
fi

#******************************************************************************
# qbuild.sh (Seq66)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 wm=4 et ft=sh
#------------------------------------------------------------------------------

