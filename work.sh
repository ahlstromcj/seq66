#!/bin/sh
#
#******************************************************************************
# work.sh (seq66)
#------------------------------------------------------------------------------
##
# \file           work.sh
# \library        seq66
# \author         Chris Ahlstrom
# \date           2026-04-23
# \update         2026-05-05
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
#     This script encapsulates some common commands.
#
#     Also see:  https://mesonbuild.com/Creating-releases.html
#
#------------------------------------------------------------------------------

LANG=C
export LANG
CYGWIN=binmode
export CYGWIN
export SEQ66_SCRIPT_EDIT_DATE="2026-05-05"
export SEQ66_LIBRARY_API_VERSION="0.99"
export SEQ66_LIBRARY_VERSION="$SEQ66_LIBRARY_API_VERSION.0"
export SEQ66="seq66"
export SEQ66_LIBRARY="$SEQ66-$SEQ66_LIBRARY_API_VERSION"

PLATFORM="UNIX"
INSTALL_PREFIX="/usr/local"         # "/usr", what about Windows?
INSTALL_LIBDIR="lib"                # "lib/x86_64-linux-gnu" on Debian

DOPORTMIDI="no"      # --portmidi. The default is rtmidi.
DOCLANG="no"         # --clang. Default is the native compiler.
DOGNU="no"           # --gnu. Default is the native compiler.
DOCLEAN="no"         # --clean
DODEBUG="yes"        # --debug. This is the default Meson build.
DODIST="no"          # --dist. Use Meson "dist" to create a package.
DOHELP="no"          # --help. Duh!
DOOPTHELP="no"       # --option-help. Duh!
DOINSTALL="no"       # --install. Requires the release be built already.
DOUNINSTALL="no"     # --uninstall. Like --install, requires sudo/root.
DOUPDATE="no"        # --update. Force a subproject update.
DOMAKE="yes"         # Default action after creating the build directory.
DOSETUP="no"         # --setup. Do the setup and then exit.
DOREMAKE="no"        # currently UNUSED
DOMAKEPDF="no"       # --pdf. Make the manual, always as a separate step.
DOPOTEXT="no"        # --potex. Use translation [NOT YET SUPPORTED].
DOPACK="no"          # --pack. Clean and create a tar-file.
DORELEASE="no"       # --release. as opposed to debug.
DOSTATIC="yes"       # --static
DOVERSION="no"       # --version. Duouble duh!
EXTRAFLAGS=""
MAKEFILE="./build/build.ninja"
TAGSTRING="pack"

#******************************************************************************
#  Brute-force options loop
#------------------------------------------------------------------------------

if test $# -ge 1 ; then

   while [ "$1" != "" ] ; do

      case "$1" in

         --portmidi)

            DOPORTMIDI="yes"
            ;;

         --clang)
            DOCLANG="yes"
            ;;

         --gnu)
            DOGNU="yes"
            ;;

         --clean)
            DOCLEAN="yes"
            DOSETUP="no"
            DOMAKE="no"
            ;;

         --help)
            DOHELP="yes"
            DOMAKE="no"
            DOSETUP="no"
            ;;

         --option-help)
            DOOPTHELP="yes"
            DOMAKE="no"
            DOSETUP="no"
            ;;

         --setup)
            DOCLEAN="yes"
            DOMAKE="no"
            DOSETUP="yes"
            ;;

         --update)
            DOUPDATE="yes"
            DOMAKE="no"
            DOSETUP="no"
            ;;

         --build | --make)
            DOMAKE="yes"
            ;;

         --rebuild | --remake)
            DOREMAKE="yes"
            DOCLEAN="yes"
            DOMAKE="yes"
            ;;

         --potext)
            DOMAKE="yes"
            DOPOTEXT="yes"
            ;;

         --install)
            DOINSTALL="yes"
            DOMAKE="no"
            DOSETUP="no"
            ;;

         --uninstall)
            DOUNINSTALL="yes"
            DOMAKE="no"
            DOSETUP="no"
            ;;

         --dist)
            DODIST="yes"
            DOMAKE="no"
            DOSETUP="no"
            ;;

         --pdf)
            DOMAKEPDF="yes"
            DOMAKE="no"
            DOSETUP="no"
            ;;

         --pack)
            DOCLEAN="yes"
            DOMAKE="no"
            DOSETUP="no"
            DOPACK="yes"
            ;;

         --debug)
            DOMAKE="yes"
            DODEBUG="yes"
            DORELEASE="no"
            ;;

         --release)
            DOMAKE="yes"
            DORELEASE="yes"
            DODEBUG="no"
            ;;

         --static)
            DORELEASE="yes"
            DODEBUG="no"
            DOSTATIC="yes"
            ;;

         --version)
            DOVERSION="yes"
            DOBOOTSTRAP="no"
            ;;

         *)
            if test "$DOPACK" = "no" ; then
               echo "? Unsupported work option; --help for more information"
               exit 1
            else
               TAGSTRING="$1"
            fi
            ;;

      esac
      shift
   done
fi

#******************************************************************************
#  Version info
#------------------------------------------------------------------------------

if test "$DOVERSION" = "yes" ; then
   echo "Seq66 version $SEQ66_LIBRARY_VERSION $SEQ66_SCRIPT_EDIT_DATE"
   exit 0
fi

#******************************************************************************
#  Help
#------------------------------------------------------------------------------

if test "$DOHELP" = "yes" ; then

   cat << E_O_F
Usage: ./work.sh [options] ($SEQ66_LIBRARY_VERSION-$SEQ66_SCRIPT_EDIT_DATE)

'work.sh' encapsulates some common operations involving Meson, builds,
packing, and version information.  Only implemented options are shown here;
there will be more to come. Some options might not work on Windows.
Many of these commands are best used when setting up the build
(i.e. do a --clean option first).

 --make or --build   Build the code in 'build'. The default operation.
 --setup             Run 'meson setup', and that's all.
 --update            Force an update of the subprojects.
 --potext            Build with Potext (light gettext) library sypport.
 --release           Build release version (Meson defaults to a debug version).
 --install           Run 'meson install' to install the library and PDF.
 --uninstall         Run 'ninja uninstall' to uninstall the library.
                     Some directories might remain; there is a error about
                     one header file not being found... strange.
 --dist              Make a Meson dist package and exit.
 --portmidi          Build using the internal PortMIDI engine instead of
                     the internal RtMIDI engine.
 --clang             Rebuild the code using the Clang compilers.
 --pdf               Build the PDF documentation. Currently done not by
                     doc/latex/tex/meson.build, but by calling
                     doc/latex/make_pdf.sh.
 --clean             Delete the usual derived files from the project. Also
                     do "git checkout doc/seq66-dev-manual.pdf"
 --rebuild           Clean the project and build from scratch.
 --pack [ tag ]      A simple quick packaging of the code; the tag goes
                     into the tarball name.
 --help              Show this help text.
 --version           Show only the version information.
 --option-help       Show the project options.

E_O_F

   exit 0

elif test "$DOOPTHELP" = "yes" ; then

   cat extras/help/options.help
   exit 0

fi

if test "$DODIST" = "yes" ; then

   meson dist
   exit 0

fi

# Make the PDF, then exit. We might get doc/latex/tex/meson.build
# to do this work at some point, but for now we use a script in
# the doc/latex directory.
#
# ENABLE_DOCS=""
# if test "$DOMAKEPDF" = "yes" ; then
#    ENABLE_DOCS="-Ddocs=true"
#    echo "Will rebuild the Seq66 User Manual...."
# fi

if test "$DOMAKEPDF" = "yes" ; then
   cd doc/latex
   ./make_pdf.sh
   cd ../..
   exit 0
fi

# This removes the work products, but leaves the README intact.

PROJECTSDIR="libraries"

if test $DOCLEAN = "yes" ; then
   rm -rf build/Desktop-Debug/
   rm -rf build/doc/
   rm -rf build/doxlatex/
   rm -rf build/include/
   rm -rf build/latex/
   rm -rf build/lib*
   rm -rf build/meson*
   rm -rf build/resources/
   rm -rf build/seq_portmidi/
   rm -rf build/seq_qt5/
   rm -rf build/seq_rtmidi/
   rm -rf build/Seq66cli/
   rm -rf build/Seq66qt5/
   rm -rf build/src/
   rm -rf build/subprojects/
   rm -rf build/tests/
   rm -rf build/uninstall/
   rm -f build/.ninja_deps
   rm -f build/.ninja_log
   rm -f build/*.h
   rm -f build/*.log
   rm -f build/*.so
   rm -f $MAKEFILE
   rm -f build/compile_commands.json
   rm -rf wipe/
   rm -f doc/dox/*.log
   rm -f doc/latex/*.log
   rm -rf build/subprojects/
   echo "Build products removed from the seq66/build directory."
   rm -rf subprojects/liblib66/
   rm -rf subprojects/potext/
   rm -rf subprojects/libcfg66
   rm -rf subprojects/libxpc66
   echo "Subproject products removed from the subprojects directory."
#  git checkout doc/seq66-dev-manual.pdf
#  echo "Previous version of developer guide restored."

# Problematic when making a release. Just remember to do it.
#
#  rm -f doc/latex/*.log
#  echo "Build products removed from the cfg66/build directory."
#  git checkout doc/cfg66-library-guide.pdf tests/data/fooout.rc
#  echo "Previous version of developer guide restored."
fi

# This is just a quick pack, with date and branch information added.

if test $DOPACK = "yes" ; then

   CURRENTDATE=$(date +%Y-%m-%d)
   CURRENTDIR=$(pwd)
   WORKINGDIR=${CURRENTDIR##/*/}       # strip all but last part of path
   PACKAGENAME="bogus"
   BRANCH=`git symbolic-ref -q HEAD 2> /dev/null`

   if test $? = 0 ; then
      ISGITBRANCH="yes"
      GITBRANCH=${BRANCH##*/}
   else
      ISGITBRANCH="no"
      GITBRANCH=""
   fi

   echo "Working directory: $WORKINGDIR"
   if test "$ISGITBRANCH" = "yes" ; then
         TARBALLNAME="$WORKINGDIR-$GITBRANCH-$CURRENTDATE-$TAGSTRING.tar.xz"
         echo "Git branch detected."
   else
         TARBALLNAME="$WORKINGDIR-$CURRENTDATE-$TAGSTRING.tar.xz"
   fi

   echo "The tar-ball to be generated is '../$TARBALLNAME'"

   cd ..
   if test -d $WORKINGDIR ; then

      tar cJf $TARBALLNAME \
 --exclude-vcs \
 --exclude=".git" \
 --exclude="moc" \
 --exclude="html" \
 --exclude="*stamp*" \
 --exclude="*.bz" \
 --exclude="*.bz2" \
 --exclude="*.gz" \
 --exclude=".*.swp" \
 --exclude="*.t" \
 --exclude="*.xz" \
 --exclude="*.zip" \
 $WORKINGDIR
      echo "Packed!"
   else
      echo "? Working directory $WORKINGDIR does not exist."
      echo "  Are you running pack from the proper directory?"
      echo "  That is what you must do."
   fi
   cd $SEQ66

fi

if test "$DOUPDATE" = "yes" ; then
   meson subprojects update
   exit 0
fi

POTEXTDEF=""
if test "$DOPOTEXT" = "yes" ; then
   POTEXTDEF="-Dpotext=true"
fi

PMIDIDEF=""
if test "$DOPORTMIDI" = "yes" ; then
   PMIDIDEF="-Denable-portmidi=true"
fi

# TODO: use a separate build directory for Clang.
#
# $ CC=clang CXX=clang++ meson setup buildclang
#
# https://mesonbuild.com/Running-Meson.html
#
# We could instead set up again with the --potex option.

if test "$DOCLANG" = "yes" ; then

   echo "Using the Clang C/C++ compilers..."
   export CC=clang
   export CXX=clang++

elif test "$DOGNU" = "yes" ; then

   echo "Using the GNU C/C++ compilers..."
   export CC=gcc
   export CXX=g++

else

   echo "Using the default/last-used C/C++ compilers..."

fi

if test "$DOSETUP" = "yes"; then
   if test "$DODEBUG" = "yes" ; then
      meson setup --buildtype=debug --default-library=static \
         $POTEXTDEF $PMIDIDEF $ENABLE_DOCS build/
      echo "... for debugging"
   else
      meson setup --buildtype=release $POTEXTDEF $PMIDIDEF $ENABLE_DOCS build/
      echo "... for release"
   fi
   exit 0
fi

if test "$DOMAKE" = "yes" ; then

   echo "Making all seq66 libraries and application..."

   NINJA_EXISTS="no"
   if test -f "$MAKEFILE" ; then
      NINJA_EXISTS="yes"
   fi

   if test "$DOREMAKE" = "yes" ; then
      if test "$NINJA_EXISTS" = "yes" ; then
         echo "$MAKEFILE exists, reconfiguring..."
         meson setup --reconfigure $POTEXTDEF $PMIDIDEF $ENABLE_DOCS build
      fi
   fi

   if test "$NINJA_EXISTS" = "no" ; then
      echo "New configuration, creating $MAKEFILE, etc...."
      if test "$DODEBUG" = "yes" ; then
         meson setup --buildtype=debug --default-library=static \
            $POTEXTDEF $PMIDIDEF $ENABLE_DOCS build/
         echo "... for debugging"
      else
         meson setup --buildtype=release $POTEXTDEF $PMIDIDEF $ENABLE_DOCS build/
         echo "... for release"
      fi
   fi

   # Could also run "meson compile" here.  The --verbose option is not
   # present on older ninjas, so we use -v here.
   #
   # Can't add this at the end, it seems to break ninja's error detection.
   #
   #     echo "# vim: ft=sh" >> make.log
   #
   # This makes output *not* go to make.log: ninja -v &> make.log

   cd build
   ninja -v > make.log
   if test $? = 0 ; then
      if test "$DODEBUG" = "yes" ; then
         echo "Debug build succeeded."
      else
         echo "Release build succeeded."
      fi
   else
      if test "$DODEBUG" = "yes" ; then
         echo "Debug build failed, check build/make.log for errors."
      else
         echo "Release build failed, check build/make.log for errors."
      fi
   fi
   cd ..

fi

# Check for root, then install. We could let meson prompt the user
# to automatically become root. Note that there are two possible locations
# for the library and pkgconfig to be installed on Linux:
#
#     -  /usr/local/lib/
#     -  /usr/local/lib/x86_64-linux-gnu/

if test "$DOINSTALL" = "yes" ; then

   USERID=$(id -u)
   if test "$USERID" = 0 ; then
      cd build
      echo "Installing the seq66 library..."
      meson install
      cd ..
   else
      echo "UID $USERID. We want you as root to install the seq66 library..."
   fi

fi

# Uninstallation is odd with Meson. The following does not work. And
# the call to ninja does not remove some directories.
#
#     cd build
#     meson --internal uninstall
#     cd ..
#
# Note that there are no man pages yet.
#
# We could 


if test "$DOUNINSTALL" = "yes" ; then

   USERID=$(id -u)
   if test "$USERID" = 0 ; then
      echo "Uninstalling the seq66 library..."
      ninja -C build uninstall
      if test "$PLATFORM" = "UNIX" ; then
         rm -rf "$INSTALL_PREFIX/include/$SEQ66_LIBRARY"
         rm -rf "$INSTALL_PREFIX/$INSTALL_LIBDIR/$POTEXT_LIBRARY"
         rm -rf "$INSTALL_PREFIX/share/doc/$SEQ66"
#        rm -rf "$INSTALL_PREFIX/man/man1/$SEQ66.1"
      fi
   else
      echo "UID $USERID. We want you as root to uninstall the seq66 library..."
   fi

fi

#******************************************************************************
# work.sh (seq66)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 wm=4 et ft=sh
#------------------------------------------------------------------------------
