#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        nsis/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-05-16
# \updates     2026-05-17
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#  The strings passed to command keyword arguments accept these substitutions:
#
#     @SOURCE_ROOT@: path to the root of the source tree. Depending on
#     backend, might be an absolute or a relative path.
#
#     @BUILD_ROOT@: path to the root of the build tree. Depending on
#     backend, might be an absolute or a relative path.
#
#     @CURRENT_SOURCE_DIR@: directory where the current processed meson.build
#     is located in. Depending on backend, might be an absolute or a relative
#     path.
#
#  $ meson compile -C build nsis-installer
#
#     The command is run from an unspecified directory, and Meson will set
#     three environment variables MESON_SOURCE_ROOT, MESON_BUILD_ROOT,
#     and MESON_SUBDIR that specify the source directory, build directory,
#     and subdirectory the target was defined in.
#
#-----------------------------------------------------------------------------

message('Processing nsis...')

seq66_makensis = find_program('makensis', required : true)
if seq66_makensis.found()
   seq66_nsis_script = '@SOURCE_ROOT@' / 'nsis' / 'MesonSetup.nsi'
   run_target('nsisinstaller',
      command : [ seq66_makensis, seq66_nsis_script ],
      depends : [ seq66cli_exe, seq66qt5_exe ]
      )
else
   error('Could not find makensis')
endif

#****************************************************************************
# meson.build (nsis)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
