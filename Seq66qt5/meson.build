#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        Seq66qt5/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-26
# \updates     2026-04-30
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library/application. See the top-level
#  meson.build file for license information.
#
#  Internal libraries:
#
#     libseq66_build
#     libsessions_build
#     seq_portmidi_build
#     seq_rtmidi_build
#     seq_qt_build
#
# Weird:
#
#  dependencies : [ qt_dep, liblib66_library_dep ],
#  link_with : [ link_libraries, seq_qt_build ],
#
#     Either way, we get these "undefined references" (UR) from the
#     qt5nsmanager class:
#
#     -  The principal c'tor
#     -  run()
#     -  close_session()
#     -  The d'tor
#
#     Rearranging qt5nsmanager and qsmainwnd yields a ton of UR's for
#     qsmainwnd functions.
#
#-----------------------------------------------------------------------------

seq66qt5_exe = executable(
   'seq66qt5',
   sources : [ 'seq66qt5.cpp' ],
   dependencies : [ seq66_qt_dep, qt_dep, liblib66_library_dep ],
   install : false   # for now
   )

#****************************************************************************
# meson.build (Seq66cli)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
