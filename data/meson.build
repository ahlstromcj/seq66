#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        data/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-05-07
# \updates     2026-05-07
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#-----------------------------------------------------------------------------

install_subdir(
   '.',
   install_dir : 'share' / seq66_project_base,
   exclude_directories : [ 'share' ],
   exclude_files : [ 'meson.build' ],
   strip_directory : true
)

install_subdir(
   './share/icons',
   install_dir : 'share' / seq66_project_base,
   exclude_directories : [ 'share' ],
   strip_directory : false
)

install_subdir(
   './share/doc',
   install_dir : 'share/doc' / seq66_project_base,
   strip_directory : true
)

install_data(
   './share/applications/seq66.desktop',
   install_dir : 'share/applications'
)

#****************************************************************************
# meson.build (data)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
