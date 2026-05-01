#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        seq_portmidi/meson.build
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

message('Processing seq_portmidi...')

libseq_portmidi_library = 'seq_portmidi'

subdir('include')
subdir('src')

if seq66_use_potext

   seq_portmidi_build = library(
      libseq_portmidi_library,
      seq66_project_base,
      seq_portmidi_sources,
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

   seq_portmidi_build = library(
      libseq_portmidi_library,
      seq_portmidi_sources,
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

if not seq66_subproj

   pkg_mod = import('pkgconfig')
   pkg_mod.generate(
      seq_portmidi_build,
      filebase : seq66_pkgconfig_name,
      version : seq66_pkg_version,
      name : seq66_project_base,
      description : seq66_pkg_description,
      install_dir : alt_pkgconfig_libdir,
      subdirs : seq66_dir,
      libraries : seq_portmidi_build,
      url : seq66_url
      )

endif

#****************************************************************************
# meson.build (seq_portmidi/seq66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
