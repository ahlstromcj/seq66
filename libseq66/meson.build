#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        libseq66/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-23
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

message('Processing libseq66...')

libseq66_library = 'seq66'

subdir('include')
subdir('src')

if seq66_use_potext

   libseq66_build = static_library(
      libseq66_library,
      libseq66_sources,
      seq66_git_tag,    # generated earlier, see seq66/include/meson.build
      install : false,  # not seq66_subproj; install_dir : seq66_libdir;
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

   libseq66_build = static_library(
      libseq66_library,
      libseq66_sources,
      seq66_git_tag,
      install : false,  # not seq66_subproj; install_dir : seq66_libdir;
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
      libseq66_build,
      filebase : seq66_pkgconfig_name,
      version : seq66_pkg_version,
      name : seq66_project_base,
      description : seq66_pkg_description,
      install_dir : alt_pkgconfig_libdir,
      subdirs : seq66_dir,
      libraries : libseq66_build,
      url : seq66_url
      )
endif
'''

#****************************************************************************
# meson.build (libseq66/seq66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
