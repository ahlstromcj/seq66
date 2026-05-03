#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        seq_qt6/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-26
# \updates     2026-05-02
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
# From 'forms', 'include', and 'src', moved to 'src/gui".
#-----------------------------------------------------------------------------

seq_ui_forms += files(
   'src/gui/qlfoframe.ui',
   'src/gui/qliveframeex.ui',
   'src/gui/qmutemaster.ui',
   'src/gui/qpatternfix.ui',
   'src/gui/qperfeditex.ui',
   'src/gui/qperfeditframe64.ui',
   'src/gui/qplaylistframe.ui',
   'src/gui/qsabout.ui',
   'src/gui/qsappinfo.ui',
   'src/gui/qsbuildinfo.ui',
   'src/gui/qseditoptions.ui',
   'src/gui/qseqeditex.ui',
   'src/gui/qseqeditframe64.ui',
   'src/gui/qseqeventframe.ui',
   'src/gui/qsessionframe.ui',
   'src/gui/qsetmaster.ui',
   'src/gui/qslivegrid.ui',
   'src/gui/qslogview.ui',
   'src/gui/qsmainwnd.ui'
)

#------------------------------------------------------------------
# Headers containing Q_OBJECT and associated with a *.ui file.
# Only such files get moc'ed.
#------------------------------------------------------------------

seq_moc_headers += files(
   'src/gui/qlfoframe.hpp',
   'src/gui/qliveframeex.hpp',
   'src/gui/qmutemaster.hpp',
   'src/gui/qpatternfix.hpp',
   'src/gui/qperfeditex.hpp',
   'src/gui/qperfeditframe64.hpp',
   'src/gui/qplaylistframe.hpp',
   'src/gui/qsabout.hpp',
   'src/gui/qsappinfo.hpp',
   'src/gui/qsbuildinfo.hpp',
   'src/gui/qseditoptions.hpp',
   'src/gui/qseqeditex.hpp',
   'src/gui/qseqeditframe64.hpp',
   'src/gui/qseqeventframe.hpp',
   'src/gui/qsessionframe.hpp',
   'src/gui/qsetmaster.hpp',
   'src/gui/qslivegrid.hpp',
   'src/gui/qslogview.hpp',
   'src/gui/qsmainwnd.hpp'
   )

seq_moc_sources += files(
   'src/gui/qlfoframe.cpp',
   'src/gui/qliveframeex.cpp',
   'src/gui/qmutemaster.cpp',
   'src/gui/qpatternfix.cpp',
   'src/gui/qperfeditex.cpp',        # see comment above
   'src/gui/qperfeditframe64.cpp',
   'src/gui/qplaylistframe.cpp',
   'src/gui/qsabout.cpp',
   'src/gui/qsappinfo.cpp',
   'src/gui/qsbuildinfo.cpp',
   'src/gui/qseditoptions.cpp',
   'src/gui/qseqeditex.cpp',
   'src/gui/qseqeditframe64.cpp',
   'src/gui/qseqeventframe.cpp',
   'src/gui/qsessionframe.cpp',
   'src/gui/qsetmaster.cpp',
   'src/gui/qslivegrid.cpp',
   'src/gui/qslogview.cpp',
   'src/gui/qsmainwnd.cpp'
)

#------------------------------------------------------------------
# More header Headers containing Q_OBJECT, but no associated
# *.ui file
#------------------------------------------------------------------

seq_q_object_headers += files(
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
   'include/qstriggereditor.hpp',
   'include/qt5nsmanager.hpp'
   )

#------------------------------------------------------------------
# The following modules do not have a corresponding .ui file,
# but they need to be moc'd.
#------------------------------------------------------------------

seq_q_object_sources += files(
   'src/qclocklayout.cpp',
   'src/qinputcheckbox.cpp',
   'src/qperfnames.cpp',
   'src/qperfroll.cpp',
   'src/qperftime.cpp',
   'src/qportwidget.cpp',
   'src/qseqdata.cpp',
   'src/qseqframe.cpp',
   'src/qseqkeys.cpp',
   'src/qseqroll.cpp',
   'src/qseqtime.cpp',
   'src/qsmaintime.cpp',
   'src/qstriggereditor.cpp',
   'src/qt5nsmanager.cpp'
)

#------------------------------------------------------------------
# Regular Qt headers, no Q_OBJECT
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
   'include/qt5_helpers.hpp'
)

#------------------------------------------------------------------
# Regular Qt source files, no Q_OBJECT
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
   )

#-----------------------------------------------------------------------------
# Import the Qt[56] module, then find Qt dependencies and specific modules.
#
# AI says that older Meson versions used qt5.preprocess() and, while still
# supported, it is recommended to use the more specific compile_moc,
# compile_ui, and compile_resources methods. Since the former does not work,
# let's try the latter.
#
# With "sources : seq_moc_sources" added to compile_moc, we get good files
# but all *.moc are empty.
#-----------------------------------------------------------------------------

qtname = 'qt6'    # qtname = 'qt5'
qtimport = import(qtname)
qt_dep = dependency(qtname, modules : [ 'Core', 'Gui', 'Widgets' ])
qt_include_dir = qt_dep.get_variable(
   pkgconfig : 'includedir', configtool : 'QT_INSTALL_HEADERS'
   )
message(qtname + ' headers are in: ' + qt_include_dir)

seq_moc_defines  = []
seq_moc_files = qtimport.compile_moc(
   headers : [ seq_moc_headers, seq_q_object_headers ],
   extra_args: seq_moc_defines #,
#  include_directories: [ seq66_includes ],
#  dependencies: qt_dep
   )

seq_ui_files = qtimport.compile_ui(
   sources : seq_ui_forms
#  preserve_paths : true
   )

'''
if seq66_use_potext

   seq_qt_build = library(
      libseq_qt5_library,
#     [ seq_qt_sources, seq_moc_sources, moc_files, ui_files ],
      [ seq_qt_sources, moc_files, ui_files ],
      install : false,                 # not seq66_subproj,
      install_dir : seq66_libdir,
      c_args : build_args,
      cpp_args : build_args,
      pic : true,
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
      pic : true,
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
'''

#****************************************************************************
# meson.build (seq_qt6/seq66)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
