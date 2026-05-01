#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        include/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-23
# \updates     2026-04-23
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#  Here we locate the headers.  They reside in:
#
#     -  include. The main include directory, for general definitions.
#     -  include/midi. Contains headers adapted from the Seq66 project.
#     -  include/seq. Contains headers refactored from the RtMidi project.
#     -  include/transport. Will contain headers related to JACK transport
#        (and perhaps other transport like Pipewire).
#
#-----------------------------------------------------------------------------

# libseq66_headers += files(
#  )

configure_file(
   input : 'seq66-config.h.in',
   output : 'seq66-config.h',
   configuration : cdata
   )

#****************************************************************************
# meson.build (include/seq66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
