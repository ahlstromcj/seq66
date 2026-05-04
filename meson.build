#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-23
# \updates     2026-05-04
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the seq66 application and library.
#
#  The seq66 library is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  The seq66 library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with the seq66 library. If not, see <https://www.gnu.org/licenses/>.
#
#  See seq66/meson.build for more information.
#
#  The seq66 library is a heavy refactoring of RtMidi 5.0.0, meant to replace
#  the seq_rtmidi library in version 2 of Seq66.
#
#  For information on meson subprojects, see
#
#     https://mesonbuild.com/Subprojects.html
#
#  Also see the "Pitchfork" project layout conventions.
#
#     https://github.com/vector-of-bool/pitchfork
#
#   cxx-pflR1
#   The Pitchfork Layout (PFL)
#   A Collection of Interesting Ideas, 2022-06-04
#
#   https://api.csswg.org/bikeshed/?force=1&url=
#       https://raw.githubusercontent.com/vector-of-bool/
#           pitchfork/develop/data/spec.bs
#
#  Note that we prefer to employ "3.1.2 Separate Header Placement".
#
# meson_version:
#
#     On mesonbuild.com: 1.4
#     Installed via pip: 1.4
#
#     If errors occur in running this build, then consider replacing your
#     distro's old meson by removing it and running:
#
#        # pip install meson
#
#     Requires version >= 1.1.0 which enables "preserve_paths" and usage of
#     the "meson.options" file.
#
# Windows:
#
#     Installed Git Bash, QtCreator with MingW usage as per Seq66, Python 3
#     from an installer (added the feature that allows > 260 character path
#     names), and Meson 1.4 from an MSI installer.  Did not install the
#     Visual Studio Community edition on our Windows virtual machine.
#
# Setup:
#
#     See the "work" bash script.
#
# Release:
#
#     See the description of the 'buildtype' option in contrib/meson.text.
#     Specify "--buildtype=release" on the meson command line.
#
# Reconfigure:
#
#        $ meson configure -Dprefix=/home/me/tmp -Dbuildtype=release build/
#
# Installation:
#
#     The library is build/libXXX66-0.1.a. We will install the XXX66 libraries
#     all in in /usr/lib/seq66-0.1 or /usr/local/lib/seq66-0.1, rather
#     than a subdirectory for each of these somewhat related libraries..
#
# Build one module only (example):
#
#     $ cd build
#     $ ninja libseq66-0.1.a.p/src_midi_busarray.cpp.o
#
# Also see:
#
#     "Porting poppler to meson"
#     https://fasterthanli.me/series/dont-shell-out/part-5
#
#     "Getting started with Meson in C++ (part 2)"
#     https://germandiagogomez.medium.com/
#        getting-started-with-meson-in-c-part-2-58150354ff17
#
# Note that C++17 is required, in order to support the usage of the
# std::make_unique<>() function and <filesystem>.
#
# However, if c_std = c17, we get an error about an implicit declaration
# of alloc() in ptlinux.c, so we set it to c11. But that gives an error
# on usleep(). So we leave this off entirely: c_std=c11'.
#
#  -Wno-deprecated -Wno-deprecated-declarations
#  'cpp_std=c++17', 'warning_level=3', 'default_library=both'
#
#-----------------------------------------------------------------------------

project('seq66',
   [ 'c', 'cpp' ],
   version : '0.99.25',
   license : 'GPLv3',
   meson_version : '>=1.4.0',
   default_options : [
      'cpp_std=c++17', 'warning_level=3', 'default_library=shared'
      ]
   )

fs = import('fs')                      # for copying resources
cc = meson.get_compiler('cpp')         # C++ compiler

#-----------------------------------------------------------------------------
# Conditional enabling of the Potext translation library.
#-----------------------------------------------------------------------------

seq66_use_potext = false
if get_option('potext')

   seq66_use_potext = true
   add_project_arguments('-DSEQ66_USE_POTEXT', language : [ 'c', 'cpp' ])

endif

#-----------------------------------------------------------------------------
# Information for this sub-project.
#-----------------------------------------------------------------------------
#
#  Tricky: the *.pc file must have a base name matching the name of the
#  subproject *.wrap file.
#
#-----------------------------------------------------------------------------

seq66_pkg_description = 'A Seq66 build using the Meson build system.'
seq66_pkg_version = meson.project_version()
seq66_version = '0.99.25'
seq66_api_version = '0.99'
seq66_info_date = '2026-05-04'
seq66_info_build_type = get_option('buildtype')
seq66_info_build_root = meson.project_build_root()
seq66_info_lib_type = get_option('default_library')
seq66_info_project_root = meson.project_source_root()
seq66_debug = get_option('buildtype').contains('debug')
seq66_project_base = 'seq66-@0@'.format(seq66_api_version)
seq66_pkgconfig_name = 'libseq66'
seq66_url = 'https://github.com/ahlstromcj/seq66'
seq66_subproj = meson.is_subproject()

if seq66_debug
   if not seq66_subproj
      add_project_arguments('-DDEBUG', language : 'cpp')
      add_project_arguments('-fno-inline-functions', language : 'cpp')
   endif
endif

#-----------------------------------------------------------------------------
# Easy access to directory options.
#-----------------------------------------------------------------------------

seq66_dir = 'seq66-@0@'.format(seq66_api_version)
seq66_prefix = get_option('prefix')
seq66_bindir = join_paths(seq66_prefix, get_option('bindir'))
seq66_includedir = join_paths(seq66_prefix, get_option('includedir'), seq66_dir)
seq66_libdir = join_paths(seq66_prefix, get_option('libdir'), seq66_dir)
seq66_datadir = join_paths(seq66_prefix, get_option('datadir'), seq66_dir)
seq66_docdir = join_paths(seq66_prefix, seq66_datadir, 'doc', seq66_dir)
libraries_top = meson.project_source_root()
libseq66_include_top = join_paths(libraries_top, 'include')

#-----------------------------------------------------------------------------
# The *.pc file goes into $libdir/seq66-0.1/pkgconfig instead of
# $libdir/pkgconfig as the meson documentation asserts.  Not sure why, but
# wiring the expected directory here works.
#-----------------------------------------------------------------------------

alt_pkgconfig_libdir = join_paths(
   seq66_prefix, get_option('libdir'), 'pkgconfig'
   )

#-----------------------------------------------------------------------------
# libseq66_headers provides the list of all headers in the project. These are
# currently all installable, public headers. They are set in
# include/meson.build.
#
# libseq66_sources provides the source-code files. They are set in
# src/meson.build.
#-----------------------------------------------------------------------------

libseq66_headers = []
libseq66_sources = []
libsessions_headers = []
libsessions_sources = []
seq_portmidi_sources = []
seq_portmidi_headers = []
seq_qt_headers = []
seq_qt_sources = []
seq_moc_headers = []
seq_moc_sources = []
seq_q_object_headers = []
seq_q_object_sources = []
seq_ui_forms = []
seq_rtmidi_sources = []
seq_rtmidi_headers = []
build_args = []

#-----------------------------------------------------------------------------
# This library has platform-dependent characteristics.
#-----------------------------------------------------------------------------

os_darwin   = false
os_freebsd  = false
os_linux    = false
os_osx      = false
os_unix     = false
os_win32    = false
use_win32   = false

app_build_issue   = 'Linux'
app_build_os      = 'Linux'
app_engine        = 'rtmidi'

if host_machine.system() == 'macosx'
   os_darwin = true
   os_osx = true
   app_build_issue = 'Mac OSX'
   app_build_os = 'OSX'
elif host_machine.system() == 'darwin'
   os_darwin = true
   os_osx = true
   app_build_issue = 'Mac OSX'
   app_build_os = 'OSX'
elif host_machine.system() == 'windows'
   os_win32 = true
   use_win32 = true
   app_build_issue = 'Windows'
   app_build_os = 'Windows'
elif host_machine.system() == 'linux'
   os_linux = true
elif host_machine.system() == 'freebsd'
   os_freebsd = true
   app_build_issue = 'FreeBSD'
   app_build_os = 'FreeBSD'
endif

os_unix = not os_win32

#-----------------------------------------------------------------------------
# Used by the seq66_xxx_version() function. Currently defined in config
# file seq66-config.h
#-----------------------------------------------------------------------------

build_args += [
   '-DSEQ66_NAME=' + '"' + meson.project_name() + '"',
   '-DSEQ66_VERSION=' + '"' + meson.project_version() + '"'
   ]

#-----------------------------------------------------------------------------
# Only make public interfaces visible
#-----------------------------------------------------------------------------

'''
if target_machine.system() == 'windows' or target_machine.system() == 'cygwin'
   build_args += '-DLIBPUBLIC="__declspec(dllexport)"'
else
   build_args += '-DLIBPUBLIC=__attribute__((visibility("default")))'
endif
'''

#-----------------------------------------------------------------------------
# Add the Seq66 information macros to include/seq66-config.h
#-----------------------------------------------------------------------------

cdata = configuration_data()

is_debug = seq66_info_build_type == 'debug'
cdata.set_quoted('api_version', seq66_api_version)
cdata.set10('debug_build', is_debug)         # built-in Meson option
cdata.set_quoted('app_build_issue', app_build_issue)
cdata.set_quoted('app_build_os', app_build_os)

#-----------------------------------------------------------------------------
# Things to settle out later.
#-----------------------------------------------------------------------------

cdata.set_quoted('app_engine', app_engine)
cdata.set_quoted('app_name', 'qseq66')
cdata.set_quoted('app_type', 'qt5')
cdata.set_quoted('client_name', 'seq66')
cdata.set_quoted('config_dir_name', 'seq66')
cdata.set_quoted('config_name', 'qseq66')
cdata.set_quoted('version_date_short', seq66_info_date)
cdata.set_quoted('version', seq66_version)

#-----------------------------------------------------------------------------
# Set up some "HAVE" macros.  By default the header file is generated in the
# "build" directory. If configure_file() is moved to include/meson.build,
# the the header goes into "build/include", which is what we want.
#-----------------------------------------------------------------------------

c_headers = [
   'assert.h',
   'ctype.h',
   'errno.h',
   'fcntl.h',
   'getopt.h',
   'glob.h',
   'limits.h',
   'pthread.h',
   'sched.h',
   'signal.h',
   'stdio.h',
   'stdlib.h',
   'string.h',
   'sys/mman.h',
   'sys/stat.h',
   'sys/types.h',
   'syslog.h',
   'time.h',
   'unistd.h'
   ]

cdata.set10('assert_h',    cc.has_header('assert.h'))
cdata.set10('ctype_h',     cc.has_header('ctype.h'))
cdata.set10('errno_h',     cc.has_header('errno.h'))
cdata.set10('fcntl_h',     cc.has_header('fcntl.h'))
cdata.set10('getopt_h',    cc.has_header('getopt.h'))
cdata.set10('glob_h',      cc.has_header('glob.h'))
cdata.set10('limits_h',    cc.has_header('limits.h'))
cdata.set10('pthread_h',   cc.has_header('pthread.h'))
cdata.set10('sched_h',     cc.has_header('sched.h'))
cdata.set10('semaphore_h', cc.has_header('semaphore.h'))
cdata.set10('signal_h',    cc.has_header('signal.h'))
cdata.set10('stdint_h',    cc.has_header('stdint.h'))
cdata.set10('stdio_h',     cc.has_header('stdio.h'))
cdata.set10('stdlib_h',    cc.has_header('stdlib.h'))
cdata.set10('string_h',    cc.has_header('string.h'))
cdata.set10('sys_mman_h',  cc.has_header('sys/mman.h'))
cdata.set10('sys_stat_h',  cc.has_header('sys/stat.h'))
cdata.set10('sys_types_h', cc.has_header('sys/types.h'))
cdata.set10('syslog_h',    cc.has_header('syslog.h'))
cdata.set10('time_h',      cc.has_header('time.h'))
cdata.set10('unistd_h',    cc.has_header('unistd.h'))

if os_unix

cdata.set10('share_h',     false)
cdata.set10('dir_h',       false)
cdata.set10('gnu_source',  false)

endif

use_alsa = false
use_jack = false
use_jack_session = false
use_jack_transport = false
use_nsm_liblo = false

if os_linux or os_freebsd

use_alsa = get_option('alsa')
use_jack = get_option('jack')
use_jack_session = get_option('jacksession')
use_jack_transport = get_option('jacktransport')
use_nsm_liblo = get_option('nsm')
use_rtmidi = get_option('rtmidi')

endif

use_portmidi = get_option('portmidi')
if use_portmidi
   use_rtmidi = false
endif

if use_jack

cdata.set10('jack_get_version_string',
   cc.has_header_symbol('jack/jack.h', 'jack_get_version_string'))

cdata.set10('jack_port_rename',
   cc.has_header_symbol('jack/jack.h', 'jack_port_rename'))

cdata.set10('jack_metadata_h', cc.has_header('jack/metadata.h'))
cdata.set10('jack_support', true)
cdata.set10('jack_session', use_jack_session)
cdata.set10('jack_transport', use_jack_transport)

endif

cdata.set10('alsa_support', use_alsa)
cdata.set10('liblo_support', use_nsm_liblo)
cdata.set10('nsm_support', use_nsm_liblo)
cdata.set10('rtmidi_support', use_rtmidi)
cdata.set10('portmidi_support', use_portmidi)
cdata.set10('windows_support', use_win32)

# Define a macro for your config.h if the header is found
#
#  foreach h : c_headers
#     if cc.has_header(h)
#        add_project_arguments(
#           '-DHAVE_@0@'.format(h.underscorify().to_upper()), language : 'c'
#           )
#     endif
#  endforeach

'''
cc.find_library ( has_headers : [ array... ])
'''

jack_headers = [
   'jack/jack.h',
   'jack/metadata.h',
   'jack/session.h',
   'jack/transport.h',
   'jack/uuid.h'
   ]

if os_osx

   core_headers = [
      'CoreAudio/HostTime.h',
      'CoreFoundation/CoreFoundation.h',
      'CoreMIDI/MIDIServices.h',
      'CoreServices/CoreServices.h'
      ]

endif

if os_win32

   pm_headers = [
      'mastermidibus_pm.hpp',
      'midibus_pm.hpp',
      'pmerrmm.h',
      'pminternal.h',
      'pmlinuxalsa.h',  # linux
      'pmlinux.h',      # linux
      'pmmac.h',
      'pmutil.h',
      'pmwinmm.h',
      'portmidi.h',
      'porttime.h'
      ]

   win_headers = [
      'dir.h',
      'io.h',
      'mmsystem.h',
      'process.h',
      'share.h',
      'sychapi.h',
      'windows.h',
      'winuser.h'
      ]

   cdata.set10('dir_h',       cc.has_header('dir.h'))
   cdata.set10('share_h',     cc.has_header('share.h'))

endif

#-----------------------------------------------------------------------------
# Each include file directory is added to the list passed to the compiler.
# We prefer to use the style '#include "cfg/header.hpp".
#-----------------------------------------------------------------------------

if use_portmidi

seq66_includes = include_directories('.',
   'include',
   'libseq66/include',
   'libsessions/include',
   'seq_portmidi/include',
   'seq_qt5/include',
   'seq_qt5/src',
   'resources'
   )

else

seq66_includes = include_directories('.',
   'include',
   'libseq66/include',
   'libsessions/include',
   'seq_rtmidi/include',
   'seq_qt5/include',
   'seq_qt5/src',
   'resources'
   )

endif

#-----------------------------------------------------------------------------
# Potential sub-projects
#-----------------------------------------------------------------------------

if seq66_use_potext

   libpotext_dep = subproject('libpotext').get_variable('libpotext_dep')
   libpotext_library_dep = dependency(
      'libpotext', fallback : [ 'libpotext', 'libpotext_dep' ]
      )

endif

liblib66_dep = subproject('liblib66').get_variable('liblib66_dep')
liblib66_library_dep = dependency(
   'liblib66', fallback : [ 'liblib66', 'liblib66_dep' ]
   )

'''

libcfg66_dep = subproject('libcfg66').get_variable('libcfg66_dep')
libcfg66_library_dep = dependency(
   'libcfg66', fallback : [ 'libcfg66', 'libcfg66_dep' ]
   )

libxpc66_dep = subproject('libxpc66').get_variable('libxpc66_dep')
libxpc66_library_dep = dependency(
   'libxpc66', fallback : [ 'libxpc66', 'libxpc66_dep' ]
   )

'''

#-----------------------------------------------------------------------------
# Dependencies on Linux
#-----------------------------------------------------------------------------
#
# ALSA:
#     The libsound version on Ubuntu 20.04 is something like 1.2.2. On
#     Ubuntua 18, 1.1.3. The current version in configure.ac of Seq66 is
#     0.9.0. Let's go with version 1.0.0.
#
# JACK:
#     The libjack-jackd2-0 version on Ubuntu 20 is something like 1.9.20.
#     On Ubuntua 18, 1.9.12. The current version in configure.ac of Seq66
#     is 0.90.0.
#
# pthreads:
#     The 'threads' dependency is required, but currently we don't care
#     about the version.
#
# Later:
#
#  if host_machine.system() == 'linux'
#     . . .
#  endif
#
#-----------------------------------------------------------------------------

if os_linux

alsa_dep = dependency(
   'alsa', version : '>=1.0.0', required : use_alsa
   )
if not alsa_dep.found()
   use_alsa = false
   warning('ALSA not found, cannot use it')
endif

jack_dep = dependency(
   'jack', version : '>=1.9.0', required : use_jack
   )
if not jack_dep.found()
   use_jack = false
   warning('JACK not found, cannot use it')
endif

liblo_dep = dependency('liblo')
if not liblo_dep.found()
   use_nsm_liblo = false
   warning('liblo not found, cannot use NSM')
endif

threads_dep = dependency('threads', required : true)

system_depends = [
   alsa_dep, jack_dep, liblo_dep, threads_dep
   ]

endif

if os_freebsd

alsa_dep = dependency(
   'alsa', version : '>=1.0.0', required : get_option('alsa')
   )

jack_dep = dependency(
   'jack', version : '>=1.9.0', required : get_option('jack')
   )

threads_dep = dependency('threads', required : true)

system_depends = [ alsa_dep, jack_dep, threads_dep ]     # liblo?

endif

if os_win32

# Yields a pkg-config error
# winmm_dep = dependency('winmm')

   winmm_dep = cc.find_library('winmm')
   system_depends = [ winmm_dep ]

endif

#-----------------------------------------------------------------------------
# Sub-directories
#-----------------------------------------------------------------------------
#
# data
# doc
# man
# resources/pixmaps
#
#-----------------------------------------------------------------------------

subdir('include')
subdir('libseq66')
subdir('libsessions')

if os_unix
   if use_portmidi
      subdir('seq_portmidi')
   else
      subdir('seq_rtmidi')
   endif
else
   subdir('seq_portmidi')
 endif

#-----------------------------------------------------------------------------
# Making the internal libraries usable as a Meson subproject requires using
# declare_dependency(), though we do not need that feature (yet).
#-----------------------------------------------------------------------------

if use_portmidi
   link_libraries = [
      libseq66_build, libsessions_build, seq_portmidi_build
      ]
else
   link_libraries = [
      libseq66_build, libsessions_build, seq_rtmidi_build
      ]
endif

seq66_dep = declare_dependency(
   include_directories : [ seq66_includes ],
   link_with : [ link_libraries ]
   )

subdir('Seq66cli')

#-----------------------------------------------------------------------------
# Make the icon pixmaps (header files in XPM format) available.
#-----------------------------------------------------------------------------

subdir('resources')

#-----------------------------------------------------------------------------
# Creates the following items:
#
#     -  qt_dep. The Core, Gui, and Widgets modules of Qt.
#     -  moc'ed files.
#     -  ui'ed files.
#     -  The seq_qt5 source files.
#     -  The seq_qt5 header files.
#        ui files, the header files
#     -  seq_qt_build. The library with built sources, mocs, uis, etc.
#
#-----------------------------------------------------------------------------

subdir('seq_qt5')

'''
seq66_qt_dep = declare_dependency(
   dependencies : [ ],
   include_directories : [ seq66_includes ],
   link_with : [ link_libraries, seq_qt_build ]
   )
'''

subdir('Seq66qt5')

#-----------------------------------------------------------------------------
# We recommemd using a recent version of meson by installing it outside the
# Linux distro's repository.  For example, meson on Ubuntu is pretty damn
# old.
#
# The choice of library type is of some import for debugging:
#
#     library:          Defaults to a shared library. The User setting is
#                       'default_library'.  See project() above.
#     shared_library:   Cannot see source files until stepped into.
#     static_library:   Can see the source files when debugging.
#     both_libraries:   Product both static and shared libraries.
#
# This one causes the linker to not see the library functions, wotta pain:
#
#     gnu_symbol_visibility: 'hidden'
#
# Also see the https://gcc.gnu.org/wiki/Visibility site.
#
# See the meson.build files in these subdirectories:
#
#     libseq66
#     libsessions
#     seq_qt5
#     seq_rtmidi
#     seq_portmidi
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Make this library usable from the system's package manager.
#-----------------------------------------------------------------------------

install_headers(libseq66_headers, subdir : seq66_dir, preserve_path : true)

#-----------------------------------------------------------------------------
# Package config
#-----------------------------------------------------------------------------

#  On Arch Linux, the .pc file is installed to /usr/local/lib/pkgconfig.
#  On Ubuntu, the .pc file is installed to
#     /usr/local/lib/x86_64-linux-gnu/pkgconfig.
#
#  We do not need this yet, but see each subdir for the pkgconfigs.
#
#-----------------------------------------------------------------------------

if not seq66_subproj

'''
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
'''

#-----------------------------------------------------------------------------
# TODO: figure out where to put and install the logo
#-----------------------------------------------------------------------------

seq66_make_pdf = get_option('docs') == true
if seq66_make_pdf
      subdir('doc')
endif

#  subdir('data')
#  subdir('man')

   doc_install_dir = get_option('docdir')
   if doc_install_dir == ''                  # empty is the default
      doc_install_dir = seq66_docdir
   endif

   install_data(
      files(
         'ChangeLog',
         'README.md',
         'data/share/doc/seq66-user-manual.pdf'
         ),
      install_dir : doc_install_dir
      )

'''
   if get_option('tests')
      subdir('tests')
   endif

   if get_option('docs')
      subdir('doc')
   endif
'''

endif

#-----------------------------------------------------------------------------
# Info
#-----------------------------------------------------------------------------

summary(
   {
      'Date' : seq66_info_date,
      'Filebase' : seq66_project_base,
      'Version' : seq66_pkg_version,
      'Base' : seq66_project_base,
      'Description' : seq66_pkg_description,
      'Source Root' : seq66_info_project_root,
      'Headers Root' : libseq66_include_top,
      'Sub Dir' : seq66_project_base,
      'Build Type' : seq66_info_build_type,
      'Is Debug' : is_debug,
      'Build Root' : seq66_info_build_root,
      'Library Type' : seq66_info_lib_type,
      'Subproject' : seq66_subproj,
      'Make PDF' : seq66_make_pdf,
      'PortMidi' : use_portmidi
   },
   section : 'Project'
   )

summary(
   {
      'prefix' : seq66_prefix,
      'includedir' : seq66_includedir,
      'bindir' : seq66_bindir,
      'libdir' : seq66_libdir,
      'datadir' : seq66_datadir,
      'docdir' : doc_install_dir,
      'appdatadir' : seq66_datadir
   },
   section : 'Seq66 Install Directories'
   )

#****************************************************************************
# meson.build (seq66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
