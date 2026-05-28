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
# \update         2026-05-28
# \version        $Revision$
# \license        $XPC_SUITE_GPL_LICENSE$
#
#     The above is modified by the following to remove even the mild GPL
#     restrictions:
#
#     Use this script in any manner whatsoever. You don't even need to give
#     me any credit.  However, keep in mind the value of the GPL in keeping
#     software and its descendant modifications available to the community for
#     all time.
#
#     This script encapsulates some common commands.
#
#     Also see:  https://mesonbuild.com/Creating-releases.html
#
#     For the values needs for the "CROSS" "PATHS", see meson.mingw.cross.
#     We're still working the issues for this.
#
#------------------------------------------------------------------------------

LANG=C
export LANG
CYGWIN=binmode
export CYGWIN
export SEQ66_SCRIPT_EDIT_DATE="2026-05-28"
export SEQ66_LIBRARY_API_VERSION="0.99"
export SEQ66_LIBRARY_VERSION="$SEQ66_LIBRARY_API_VERSION.0"
export SEQ66="seq66"
export SEQ66_LIBRARY="$SEQ66-$SEQ66_LIBRARY_API_VERSION"

# Settings.

BASE_BUILD_DIR="build"              # 'seq66/build'
BUILD_DIR="$BASE_BUILD_DIR/cc"      # "native" compiler (CC/CXX) build
BUILD_TYPE="release"
CROSS_PKG_PATH="/usr/lib/pkgconfig" # TO DO TO DO
EXTRAFLAGS=""
# INSTALL_LIBDIR="lib"                # "lib/x86_64-linux-gnu" on Debian
# INSTALL_PREFIX="/usr/local"         # "/usr", what about Windows?
MAKEFILE="$BUILD_DIR/build.ninja"
MAKELOG="make.log"
PLATFORM="UNIX"
PMIDIDEF=""
POTEXTDEF=""
TAGSTRING="pack"
NOJACK=""                           # --no-jack to disable
NOJACKSESSION="-Djacksession=false" # --jack-session to enable
NOJACKTRANSPORT=""                  # --no-jack-transport to disable
NONSM=""                            # --no-nsm to disable

# Flags.

DOCLANG="no"            # --clang. Default is the native compiler.
DOCLEAN="no"            # --clean
DOCROSS="no"            # --cross. Build for Windows using a cross-file.
DODEBUG="no"            # --debug. This is the default Meson build.
DODIST="no"             # --dist. Use Meson "dist" to create a package.
DOGNU="no"              # --gnu. Default is the native compiler.
DOHELP="no"             # --help. Duh!
DOINSTALL="no"          # --install. Requires the release be built already.
DOMAKE="yes"            # Default action after creating the build directory.
DOMAKEPDF="no"          # --pdf. Make the manual, always as a separate step.
DONSIS="no"             # --nsis. Make an NSIS Windows installer.
DOOPTHELP="no"          # --option-help. Duh!
DOPACK="no"             # --pack. Clean and create a tar-file.
DOPORTMIDI="no"         # --portmidi. The default is rtmidi.
DOPOTEXT="no"           # --potext. Use translation [NOT YET SUPPORTED].
DORELEASE="yes"         # --release. as opposed to debug.
DOREMAKE="no"           # currently UNUSED
DOSETUP="no"            # --setup. Do the setup and then exit.
DOSTATIC="yes"          # Not an option for Seq66... Mandatory.
DOUNINSTALL="no"        # --uninstall. Like --install, requires sudo/root.
DOUPDATE="no"           # --update. Force a subproject update.
DOVERSION="no"          # --version. Double duh!

#******************************************************************************
#  Brute-force options loop
#------------------------------------------------------------------------------

get_options () {
   if test $# -ge 1 ; then
      while [ "$1" != "" ] ; do

         case "$1" in

            --make)
               DOMAKE="yes"
               ;;

            --build)
               if test "$DOCLEAN" = "no" ; then
                  DOMAKE="yes"
               fi
               shift
               case $1 in
                  -*)
                     continue
                  ;;
               esac
               BUILD_DIR="$BASE_BUILD_DIR/$1"
               ;;

            --cross)
               DOMAKE="yes"
               DOCROSS="yes"
               BUILD_DIR="$BASE_BUILD_DIR/cross"
               MAKEFILE="$BUILD_DIR/build.ninja"
               MAKELOG="$BUILD_DIR/make.log"
               CROSSENVSET="PKG_CONFIG_PATH=$CROSS_PKG_PATH:$PKG_CONFIG_PATH"
               export $CROSSENVSET
               echo "CROSSENVSET: PKG_CONFIG_PATH=$PKG_CONFIG_PATH"
               ;;

            --clean)
               DOCLEAN="yes"
               DOSETUP="no"
               DOMAKE="no"
               ;;

            --dist)
               DODIST="yes"
               DOMAKE="no"
               DOSETUP="no"
               ;;

            --clang)
               DOCLANG="yes"
               echo "Using the Clang C/C++ compilers..."
               export CC=clang
               export CXX=clang++
               BUILD_DIR="$BASE_BUILD_DIR/clang"
               MAKEFILE="$BUILD_DIR/build.ninja"
               ;;

            --gnu | --gcc)
               DOGNU="yes"
               echo "Using the GNU C/C++ compilers..."
               export CC=gcc
               export CXX=g++
               BUILD_DIR="$BASE_BUILD_DIR/gcc"
               MAKEFILE="$BUILD_DIR/build.ninja"
               ;;

            --help)
               DOHELP="yes"
               DOMAKE="no"
               DOSETUP="no"
               ;;

            --install)
               DOINSTALL="yes"
               DOMAKE="no"
               DOSETUP="no"
               ;;

            --option-help)
               DOOPTHELP="yes"
               DOMAKE="no"
               DOSETUP="no"
               ;;

            --pack)
               DOCLEAN="yes"
               DOMAKE="no"
               DOSETUP="no"
               DOPACK="yes"
               ;;

            --pdf)
               DOMAKEPDF="yes"
               DOMAKE="no"
               DOSETUP="no"
               ;;

            --nsis)
               DONSIS="yes"
               DOMAKE="no"
               DOSETUP="no"
               ;;

            --rtmidi)
               BUILD_DIR="$BASE_BUILD_DIR/rtmidi"
               MAKEFILE="$BUILD_DIR/build.ninja"
               ;;

            --portmidi)
               DOPORTMIDI="yes"
               PMIDIDEF="-Dportmidi=true -Drtmidi=false"
               BUILD_DIR="$BASE_BUILD_DIR/portmidi"
               MAKEFILE="$BUILD_DIR/build.ninja"
               ;;

            --potext)
               DOMAKE="yes"
               DOPOTEXT="yes"
               POTEXTDEF="-Dpotext=true"
               ;;

            --rebuild | --remake)
               DOREMAKE="yes"
               DOCLEAN="yes"
               DOMAKE="yes"
               ;;

            --setup)
               DOCLEAN="yes"
               DOMAKE="no"
               DOSETUP="yes"
               ;;

            --uninstall)
               DOUNINSTALL="yes"
               DOMAKE="no"
               DOSETUP="no"
               ;;

            --update)
               DOUPDATE="yes"
               DOMAKE="no"
               DOSETUP="no"
               ;;

            --windows)
               BUILD_DIR="$BASE_BUILD_DIR/windows"
               MAKEFILE="$BUILD_DIR/build.ninja"
               ;;

            --debug)
               if test "$DOCLEAN" = "no" ; then
                  DOMAKE="yes"
               fi
               DODEBUG="yes"
               DORELEASE="no"
               BUILD_TYPE="debug"
               BUILD_DIR="$BASE_BUILD_DIR/debug"
               MAKEFILE="$BUILD_DIR/build.ninja"
               ;;

            --release)
               DOMAKE="yes"
               DORELEASE="yes"
               DODEBUG="no"
               BUILD_TYPE="release"
               ;;

            --no-jack)
               NOJACK="-Djack=false"
               ;;

            --jack-session)
               NOJACKSESSION="-Djacksession=true"
               ;;

            --no-jack-transport)
               NOJACKTRANSPORT="-Djacktransport=false"
               ;;

            --no-nsm)
               NONSM="-Dnsm=false"
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
}

#******************************************************************************
#  Version info
#------------------------------------------------------------------------------

show_version () {
   echo "Seq66 version $SEQ66_LIBRARY_VERSION $SEQ66_SCRIPT_EDIT_DATE"
}

#******************************************************************************
#  Help
#------------------------------------------------------------------------------

show_help () {
      cat << E_O_F
Usage: ./work.sh [options] ($SEQ66_LIBRARY_VERSION-$SEQ66_SCRIPT_EDIT_DATE)

'work.sh' encapsulates common operations involving Meson, builds,
packing, and version information.  Some options might not work on Windows.
Many of these commands are best used when setting up the build
(i.e. do a --clean option first).

 --make              Set up and build the code in 'build'. Default operation.
 --build [dir]       Same as --make, but if given, the build directory is
                     'build/dir'.
 --cross             Set up to build a Windows executable, and build it.
                     Not workable yet; see mingw-qt-build.text.
 --setup             Run 'meson setup', and that's all.
 --update            Force an update of the subprojects.
 --potext            Build with Potext (light gettext) library sypport.
 --release           Build release version (the default).
 --debug             Build debug version. Always builds in 'build/debug'.
 --install           Run 'meson install' to install Seq66 and the PDF manual.
 --uninstall         Run 'ninja uninstall' to uninstall the library.
                     Some directories might remain; there is a error about
                     one header file not being found... strange.
 --dist              Make a Meson dist package and exit.
 --rtmidi            This flag builds in 'build/rtmidi' rather than
                     'build/cc'. Use when trying portmidi at the same time
 --portmidi          Build using the internal PortMIDI engine instead of
                     the internal RtMIDI engine. Builds in 'build/portmide',
                     not 'build/cc'
 --windows           On Windows, set up 'build/windows' and build it using
                     the mingw/Qt install. Build directory is 'build/windows'.
 --clang             Rebuild the code using the Clang compilers. Exports CC
                     and CXX. Build directory is 'build/clang'.
 --gnu, --gcc        Use the GNU compilers (the default on Linux). Exports CC
                     and CXX. Build directory is 'build/gcc'. If the
                     compiler is not specified, the build is in 'build/cc'.
 --pdf               Build the PDF documentation. Currently done not by
                     doc/latex/tex/meson.build, but by calling
                     doc/latex/make_pdf.sh.
 --nsis              Use NSIS to make a Windows installer on Linux.
 --clean             Delete the usual derived files from the project. Also
                     do "git checkout doc/seq66-dev-manual.pdf"
 --rebuild           Clean the project and build from scratch.
 --jack-session      Enable the usage of JACK Session, which is deprecated.
 --no-jack           Disable the usage of JACK (in Linux).
 --no-jack-transport Disable the usage of JACK Transport.
 --no-nsm            Disable the usage of NSM.
 --pack [ tag ]      A simple quick packaging of the code; the tag goes
                     into the tarball name.
 --help              Show this help text.
 --version           Show only the version information.
 --option-help       Show the project options available. They can be used
                     as "-Doption=value" when using meson directly
                     (instead of through the work.sh script).
E_O_F
      exit 0
}

#******************************************************************************
#  PDF
#------------------------------------------------------------------------------

make_pdf () {
   cd doc/latex
   ./make_pdf.sh
   cd ../..
}

#******************************************************************************
# Remove all the sub-directories of ./build. All build products
# go into a sub-directory.
#------------------------------------------------------------------------------

clean_build () {

   for DIR in $BASE_BUILD_DIR/*/ ; do
      echo "Deleting $DIR"
      rm -rf $DIR
   done

   rm -f doc/dox/*.log
   rm -f doc/latex/*.log
   echo "Build products removed from the $SEQ66/build sub-directories."
   rm -rf subprojects/liblib66/
   rm -rf subprojects/potext/          # available, but code not prep'ed
   rm -rf subprojects/libcfg66         # not yet in use
   rm -rf subprojects/libxpc66         # not yet in use
   rm -rf subprojects/librtl66         # not yet in use
   echo "Subproject downloaded libraries removed from 'subprojects'."

#  git checkout data/share/doc/seq66-dev-manual.pdf
#  echo "Previous version of developer guide restored."
#  Problematic when making a release. Just remember to do it.

}

make_pack () {
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
}

make_projects () {
   echo "Making all $SEQ66 libraries and application..."
   NINJA_EXISTS="no"
   if test -f "$MAKEFILE" ; then
      NINJA_EXISTS="yes"
   fi
   if test "$DOREMAKE" = "yes" ; then
      if test "$NINJA_EXISTS" = "yes" ; then
         echo "$MAKEFILE exists, reconfiguring..."
         meson setup --reconfigure $MOPTS
      fi
   fi
   if test "$NINJA_EXISTS" = "no" ; then
      echo "New configuration, creating $MAKEFILE, etc...."
      if test "$DODEBUG" = "yes" ; then
         meson setup --default-library=static $MOPTS
         echo "... for debugging"
      else
         meson setup $MOPTS
         echo "... for release"
      fi
   fi

   # Could also run "meson compile" here.  The --verbose option is not
   # present on older ninjas, so we use -v here.
   #
   # Can't add this at the end, it seems to break ninja's error detection.

   cd $BUILD_DIR
   ninja -v > $MAKELOG
   if test $? = 0 ; then
      if test "$DODEBUG" = "yes" ; then
         echo "Debug build in $BUILD_DIR succeeded."
      else
         echo "Release build in $BUILD_DIR succeeded."
      fi
   else
      if test "$DODEBUG" = "yes" ; then
         echo "Debug build failed, check $MAKELOG for errors."
      else
         echo "Release build failed, check $MAKELOG for errors."
      fi
   fi
   cd ..
}

#******************************************************************************
# Check for root, then install. We could let meson prompt the user
# to automatically become root. Note that there are two possible locations
# for the library and pkgconfig to be installed on Linux:
#
#     -  /usr/local/lib/
#     -  /usr/local/lib/x86_64-linux-gnu/
#
# What's weird is we seem to need "meson install", yet need
# "ninja ... uninstall".
#
#------------------------------------------------------------------------------

install_project () {
   USERID=$(id -u)
   if test "$USERID" = 0 ; then
      cd $BUILD_DIR
      echo "Installing the $SEQ66 application, data, and documents..."
      meson install                    # ninja -C $BUILD_DIR install
      cd ..
   else
      echo "UID $USERID. We want you as root to install the $SEQ66 library..."
   fi
}

#******************************************************************************
# Uninstallation is odd with Meson. The following does not work. And
# the call to ninja does not remove some directories.
#
#     cd build
#     meson --internal uninstall
#     cd ..
#
#------------------------------------------------------------------------------

uninstall_project () {
   USERID=$(id -u)
   if test "$USERID" = 0 ; then
      echo "Uninstalling the $SEQ66 application, data, and documents..."
      ninja -C $BUILD_DIR uninstall    # meson uninstall
#     if test "$PLATFORM" = "UNIX" ; then
#        rm -rf "$INSTALL_PREFIX/include/$SEQ66_LIBRARY"
#        rm -rf "$INSTALL_PREFIX/$INSTALL_LIBDIR/$POTEXT_LIBRARY"
#        rm -rf "$INSTALL_PREFIX/share/doc/$SEQ66"
#        rm -rf "$INSTALL_PREFIX/man/man1/$SEQ66.1"
#     fi
   else
      echo "UID $USERID. We want you as root to uninstall the $SEQ66 library..."
   fi
}

#******************************************************************************
#  Parse the command-line options.
#------------------------------------------------------------------------------

get_options $*

#******************************************************************************
#  Version info
#------------------------------------------------------------------------------

if test "$DOVERSION" = "yes" ; then
   show_version
   exit 0
fi

#******************************************************************************
#  Help
#------------------------------------------------------------------------------

if test "$DOHELP" = "yes" ; then

   show_help
   exit 0

elif test "$DOOPTHELP" = "yes" ; then

   cat extras/help/options.help
   exit 0

fi

if test "$DODIST" = "yes" ; then

   meson dist
   exit 0

fi

#******************************************************************************
# Make the PDF, then exit. We might get doc/latex/tex/meson.build
# to do this work at some point, but for now we use a script in
# the doc/latex directory.
#
# ENABLE_DOCS=""
# if test "$DOMAKEPDF" = "yes" ; then
#    ENABLE_DOCS="-Ddocs=true"
#    echo "Will rebuild the Seq66 User Manual...."
# fi
#------------------------------------------------------------------------------

if test "$DOMAKEPDF" = "yes" ; then
   make_pdf
   exit 0
fi

# This removes the work products, but leaves the README intact.

if test $DOCLEAN = "yes" ; then
   clean_build
fi

# This is just a quick pack, with date and branch information added.

if test $DOPACK = "yes" ; then
   make_pack
   exit 0
fi

if test "$DOUPDATE" = "yes" ; then
   meson subprojects update
   exit 0
fi

MOPTS="--buildtype=$BUILD_TYPE $POTEXTDEF $PMIDIDEF $BUILD_DIR $NOJACK $NOJACKSESSION $NOJACKTRANSPORT $NONSM"

#******************************************************************************
# --setup. This section does only a setup, then exits.
#------------------------------------------------------------------------------

if test "$DOSETUP" = "yes"; then
   if test "$DODEBUG" = "yes" ; then
      meson setup --default-library=static $MOPTS
      echo "... for debugging"
   else
      meson setup $MOPTS
      echo "... for release"
   fi
   exit 0
fi

#******************************************************************************
# --cross. This function sets options appropriate for Windows builds,
#          does the build, and exits.
#
# EXPERIMENTAL.
#
# Currently does not support the Potext library or a debug build.
#------------------------------------------------------------------------------

if test "$DOCROSS" = "yes" ; then

   CROSSOPTS="-Dportmidi=true -Dpotext=false -Djack=false -Dnsm=false"
   CROSSFILE="--cross-file meson.mingw.cross"
   meson setup $BUILD_DIR --buildtype=$BUILD_TYPE $CROSSOPTS $CROSSFILE
   if test $? = 0 ; then
      meson compile -C $BUILD_DIR > $MAKELOG
      if test $? = 0 ; then
         echo "Cross-build in $BUILD_DIR succeeded."
         exit 0
      else
         echo "Cross-build failed, check $MAKELOG for errors."
         exit 1
      fi
   fi
fi

if test "$DONSIS" = "yes" ; then
   meson compile -C $BUILD_DIR nsisinstaller
   exit 0
fi

if test "$DOMAKE" = "yes" ; then
   make_projects
   exit 0
fi

#******************************************************************************
# Check for root, then install. We could let meson prompt the user
# to automatically become root. Note that there are two possible locations
# for the library and pkgconfig to be installed on Linux:
#
#     -  /usr/local/lib/
#     -  /usr/local/lib/x86_64-linux-gnu/
#------------------------------------------------------------------------------

if test "$DOINSTALL" = "yes" ; then
   install_project
   exit 0
fi

#******************************************************************************
# Uninstallation is odd with Meson. The following does not work. And
# the call to ninja does not remove some directories.
#
#     cd $BUILD_DIR
#     meson --internal uninstall
#     cd ..
#
# Note that there are no man pages yet.
#------------------------------------------------------------------------------

if test "$DOUNINSTALL" = "yes" ; then
   uninstall_project
fi

#******************************************************************************
# work.sh (seq66)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 wm=4 et ft=sh
#------------------------------------------------------------------------------
