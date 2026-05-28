#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        seq_rtmidi/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-25
# \updates     2026-05-08
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#  We use a static library since we're not installing the libraries;
#  using a shared library build causes undefined references at link
#  time.
#
#-----------------------------------------------------------------------------

message('Processing seq_rtmidi...')

libseq_rtmidi_library = 'seq_rtmidi'

subdir('include')
subdir('src')

if seq66_use_potext

   seq_rtmidi_build = static_library(
      libseq_rtmidi_library,
      seq_rtmidi_sources,
      install : false,
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

   seq_rtmidi_build = static_library(
      libseq_rtmidi_library,
      seq_rtmidi_sources,
      install : false,
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

#-----------------------------------------------------------------------------
# Since this is an internal helper library, it will not be installed.
#-----------------------------------------------------------------------------

'''
if not seq66_subproj
   pkg_mod = import('pkgconfig')
   pkg_mod.generate(
      seq_rtmidi_build,
      filebase : seq66_pkgconfig_name,
      version : seq66_pkg_version,
      name : seq66_project_base,
      description : seq66_pkg_description,
      install_dir : alt_pkgconfig_libdir,
      subdirs : seq66_dir,
      libraries : seq_rtmidi_build,
      url : seq66_url
      )
endif
'''

#****************************************************************************
# meson.build (seq_rtmidi/seq66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
