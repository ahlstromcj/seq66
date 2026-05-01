#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        seq_qt6/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-26
# \updates     2026-04-29
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#  moc_headers:         Files using QOBJECT.
#  moc_extra_arguments: [ '-DMAKE_MOC_COMPILE' ] as an example.
#  ui_files:            Files *.ui in the forms directory.
#  uic_extra_arguments: [ ] ???
#  qrc_files:           Resource files *.qrc. This project doesn't use them.
#  qresources:          Same,
#  include_directories: TO DO.
#  dependencies:        qt_dep, see below.
#
# On Debian Sid:
#
#     /usr/include/x86_64-linux-gnu/qt6
#     /usr/include/x86_64-linux-gnu/qt5
#
# Arch Linux:
#
#     /usr/include/qt6
#     /usr/include/qt
#
# Windows (MingW):
#
#     TODO
#
#-----------------------------------------------------------------------------

message('Processing seq_qt6...')

libseq_qt5_library = 'seq_qt5'

#-----------------------------------------------------------------------------
# subdir('forms')
#-----------------------------------------------------------------------------

seq_ui_forms += files(
   'forms/qlfoframe.ui',
   'forms/qliveframeex.ui',
   'forms/qmutemaster.ui',
   'forms/qpatternfix.ui',
   'forms/qperfeditex.ui',
   'forms/qperfeditframe64.ui',
   'forms/qplaylistframe.ui',
   'forms/qsabout.ui',
   'forms/qsappinfo.ui',
   'forms/qsbuildinfo.ui',
   'forms/qseditoptions.ui',
   'forms/qseqeditex.ui',
   'forms/qseqeditframe64.ui',
   'forms/qseqeventframe.ui',
   'forms/qsessionframe.ui',
   'forms/qsetmaster.ui',
   'forms/qslivegrid.ui',
   'forms/qslogview.ui',
   'forms/qsmainwnd.ui'
)

#-----------------------------------------------------------------------------
# subdir('include')
#-----------------------------------------------------------------------------

#------------------------------------------------------------------
# Regular headers
#------------------------------------------------------------------

seq_qt_headers += files(
   'include/gui_palette_qt5.hpp',
   'include/palettefile.hpp',
   'include/qbase.hpp',
   'include/qeditbase.hpp',
   'include/qloopbutton.hpp',
   'include/qperfbase.hpp',
   'include/qscrollmaster.h',
   'include/qscrollslave.h',
   'include/qseqbase.hpp',
   'include/qseventslots.hpp',
   'include/qslivebase.hpp',
   'include/qslotbutton.hpp',
   'include/qt5_helper.h',
   'include/qt5_helpers.hpp',
)

#-----------------------------------------------------------------------------
# subdir('src')
#-----------------------------------------------------------------------------

#------------------------------------------------------------------
# Moc headers.
#
# The first group has files contain Q_OBJECT, but do not get
# moc'ed. Why? Each file has /no/ *.ui file.
#
# seq_no_moc_headers += files(
#
#------------------------------------------------------------------

seq_qt_headers += files(
   'include/qclocklayout.hpp',
   'include/qinputcheckbox.hpp',
   'include/qperfnames.hpp',
   'include/qperfroll.hpp',
   'include/qperftime.hpp',
   'include/qportwidget.hpp',
   'include/qseqdata.hpp',
   'include/qseqframe.hpp',
   'include/qseqkeys.hpp',
   'include/qseqroll.hpp',
   'include/qseqtime.hpp',
   'include/qsmaintime.hpp',
   'include/qstriggereditor.hpp'
   )

seq_moc_headers += files(
   'include/qlfoframe.hpp',
   'include/qliveframeex.hpp',
   'include/qmutemaster.hpp',
   'include/qpatternfix.hpp',
   'include/qperfeditex.hpp',
   'include/qperfeditframe64.hpp',
   'include/qplaylistframe.hpp',
   'include/qsabout.hpp',
   'include/qsappinfo.hpp',
   'include/qsbuildinfo.hpp',
   'include/qseditoptions.hpp',
   'include/qseqeditex.hpp',
   'include/qseqeditframe64.hpp',
   'include/qseqeventframe.hpp',
   'include/qsessionframe.hpp',
   'include/qsetmaster.hpp',
   'include/qslivegrid.hpp',
   'include/qslogview.hpp',
   'include/qsmainwnd.hpp',
   'include/qt5nsmanager.hpp'
)

#------------------------------------------------------------------
# These files do not contain Q_OBJECT 
#------------------------------------------------------------------

seq_qt_sources += files(
   'src/gui_palette_qt5.cpp',
   'src/palettefile.cpp',
   'src/qbase.cpp',
   'src/qeditbase.cpp',
   'src/qloopbutton.cpp',
   'src/qperfbase.cpp',
   'src/qscrollmaster.cpp',
   'src/qscrollslave.cpp',
   'src/qseqbase.cpp',
   'src/qseventslots.cpp',
   'src/qslivebase.cpp',
   'src/qslotbutton.cpp',        # Derived from QPushButton
   'src/qt5_helpers.cpp',

   #--------------------------------------------------------------
   # The following modules do not have a corresponding .ui file.
   #--------------------------------------------------------------

   'src/qclocklayout.cpp',
   'src/qinputcheckbox.cpp',
   'src/qperfnames.cpp',
   'src/qperfroll.cpp',
   'src/qperftime.cpp',
   'src/qportwidget.cpp',        # Derived from QWidget
   'src/qseqdata.cpp',
   'src/qseqframe.cpp',
   'src/qseqkeys.cpp',
   'src/qseqroll.cpp',
   'src/qseqtime.cpp',
   'src/qsmaintime.cpp',
#  'src/qt5nsmanager.cpp',       # here, yields many qsmainwnd UR's
   'src/qstriggereditor.cpp'
)

seq_moc_sources += files(
   'src/qlfoframe.cpp',
   'src/qliveframeex.cpp',
   'src/qmutemaster.cpp',
   'src/qpatternfix.cpp',
   'src/qperfeditex.cpp',        # see comment above
   'src/qperfeditframe64.cpp',
   'src/qplaylistframe.cpp',
   'src/qsabout.cpp',
   'src/qsappinfo.cpp',
   'src/qsbuildinfo.cpp',
   'src/qseditoptions.cpp',
   'src/qseqeditex.cpp',
   'src/qseqeditframe64.cpp',
   'src/qseqeventframe.cpp',
   'src/qsessionframe.cpp',
   'src/qsetmaster.cpp',
   'src/qslivegrid.cpp',
   'src/qslogview.cpp',
   'src/qt5nsmanager.cpp',
   'src/qsmainwnd.cpp'
)

#-----------------------------------------------------------------------------
# Import the Qt[56] module, then find Qt dependencies and specific modules.
#
# AI says that older Meson versions used qt5.preprocess() and, while still
# supported, it is recommended to use the more specific compile_moc,
# compile_ui, and compile_resources methods. Since the former does not work,
# let's try the latter.
#-----------------------------------------------------------------------------

qtname = 'qt6'
qtimport = import(qtname)
qt_dep = dependency(qtname, modules : [ 'Core', 'Gui', 'Widgets' ])
qt_include_dir = qt_dep.get_variable(
   pkgconfig : 'includedir', configtool : 'QT_INSTALL_HEADERS'
   )
message(qtname + ' headers are in: ' + qt_include_dir)

moc_files = qtimport.compile_moc(
   headers : seq_moc_headers,
   sources : seq_moc_sources,
   extra_args: [ ],
   include_directories: [ seq66_includes ],
   dependencies: qt_dep
   )

ui_files = qtimport.compile_ui(
   sources : seq_ui_forms,
   preserve_paths : true
   )

if seq66_use_potext

   seq_qt_build = library(
      libseq_qt5_library,
      [ seq_qt_sources, moc_files, ui_files ],
      install : false,                 # not seq66_subproj,
      install_dir : seq66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      dependencies : [
         system_depends, liblib66_library_dep, qt_dep,
#        libcfg66_library_dep,
#        libxpc66_library_dep,
         libpotext_library_dep
         ],
      include_directories : [ seq66_includes ]
      )

else

   seq_qt_build = library(
      libseq_qt5_library,
      [ seq_qt_sources, moc_files, ui_files ],
      install : false,                 # not seq66_subproj,
      install_dir : seq66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      dependencies : [
         system_depends, liblib66_library_dep, qt_dep,
#        libcfg66_library_dep,
#        libxpc66_library_dep
         ],
      include_directories : [ seq66_includes ]
      )

# link_args?
# link_early_args?
# pic?
# prelink

endif

if not seq66_subproj

   pkg_mod = import('pkgconfig')
   pkg_mod.generate(
      seq_qt_build,
      filebase : seq66_pkgconfig_name,
      version : seq66_pkg_version,
      name : seq66_project_base,
      description : seq66_pkg_description,
      install_dir : alt_pkgconfig_libdir,
      subdirs : seq66_dir,
      libraries : seq_qt_build,
      url : seq66_url
      )

endif

#****************************************************************************
# meson.build (seq_qt6/seq66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
