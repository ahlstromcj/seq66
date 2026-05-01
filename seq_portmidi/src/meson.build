#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        seq_portmidi/src/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-25
# \updates     2026-04-25
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
# Mac:      Currently unbuilt, untested, we have no Mac computer.
# Windows:  Not yet ready in the Meson build.
#
#-----------------------------------------------------------------------------

if os_linux

seq_portmidi_sources += files(
   'mastermidibus.cpp',
   'midibus.cpp',
   'pmlinuxalsa.c',
   'pmlinux.c',
   'pmutil.c',
   'portmidi.c',
   'porttime.c',
   'ptlinux.c'
)

elif os_win32

seq_portmidi_sources += files(
   'mastermidibus.cpp',
   'midibus.cpp',
   'pmerrmm.c',
   'pmutil.c',
   'portmidi.c',
   'porttime.c',
   'pmwin.c',
   'pmwinmm.c',
   'ptwinmm.c'
)

elif os_osx

seq_portmidi_sources += files(
   'mastermidibus.cpp',
   'midibus.cpp',
   'pmutil.c',
   'portmidi.c',
   'porttime.c',
   'pmmac.c',
   'pmmacosxcm.c',
   'ptmacosx_cf.c',
   'ptmacosx_mach.c'
)

endif

#****************************************************************************
# meson.build (seq_portmidi/src)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
