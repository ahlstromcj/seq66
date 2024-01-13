#!/bin/sh
#
#******************************************************************************
# autogen.sh (Seq66)
#------------------------------------------------------------------------------
##
# \file           autogen.sh
# \library        Seq66
# \author         Chris Ahlstrom
# \date           2024-01-12
# \update         2024-01-13
# \version        $Revision$
# \license        Whatever
#
#     Use this script in any manner whatsoever.  You don't even need to give
#     me any credit.  However, keep in mind the value of the GPL in keeping
#     software and its descendant modifications available to the community for
#     all time.  Uses basic shell commands instead of bashisms.
#
#------------------------------------------------------------------------------


aclocal -I m4 --install
if test $? = 0 ; then
   echo "aclocal finished"
   autoconf
   if test $? = 0 ; then
      echo "autoconf finished"
      autoheader
      if test $? = 0 ; then
         echo "autoheader finished"
         libtoolize --automake --force --copy
         if test $? = 0 ; then
            echo "libtoolize finished"
            automake --foreign --add-missing --copy
            if test $? = 0 ; then
               echo "automake finished"
            else
               echo "automake failed; is it installed?"
            fi
         else
            echo "libtoolize failed; is it installed?"
         fi
      else
         echo "autoheader failed"
      fi
   else
      echo "autoconf failed; is autoconf installed?"
   fi
else
   echo "aclocal failed; are autoconf and autoconf-archive installed?"
fi

echo "If all steps succeeded, run ./configure with desired parameters."


#******************************************************************************
# autogen.sh (Seq66)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 wm=4 et ft=sh
#------------------------------------------------------------------------------
