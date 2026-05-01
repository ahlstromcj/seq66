#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        libsessions/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-25
# \updates     2026-04-29
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#-----------------------------------------------------------------------------

message('Processing libsessions...')

libsessions_library = 'sessions'

subdir('include')
subdir('src')

if seq66_use_potext

   libsessions_build = library(
      libsessions_library,
      libsessions_sources,
      install : false,                 # not seq66_subproj,
      install_dir : seq66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      dependencies : [
         system_depends, liblib66_library_dep,
#        libcfg66_library_dep,
#        libxpc66_library_dep,
         libpotext_library_dep
         ],
      include_directories : [ seq66_includes ]
      )

else

   libsessions_build = library(
      libsessions_library,
      libsessions_sources,
      install : false,                 # not seq66_subproj,
      install_dir : seq66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      dependencies : [
         system_depends, liblib66_library_dep
#        libcfg66_library_dep,
#        libxpc66_library_dep
         ],
      include_directories : [ seq66_includes ]
      )

endif

#****************************************************************************
# meson.build (libsessions/seq66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
