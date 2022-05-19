#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_qt5 library
# \author     	Chris Ahlstrom
# \date       	2018-01-01
# \update      2022-05-19
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_qt5 C/C++
# 		library.
#
# Removed:
#
#     qseqeditframe.cpp
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2 subdir-objects
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#******************************************************************************
# CLEANFILES
#------------------------------------------------------------------------------
#
#  Do not forget to clean the moc/uic/rcc generated files.
#
#------------------------------------------------------------------------------

CLEANFILES = *.gc* *.moc.cpp ../forms/*.ui.h
MOSTLYCLEANFILES = *~

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = 

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@
GIT_VERSION = @GIT_VERSION@

SEQ66_API_MAJOR = @SEQ66_API_MAJOR@
SEQ66_API_MINOR = @SEQ66_API_MINOR@
SEQ66_API_PATCH = @SEQ66_API_PATCH@
SEQ66_API_VERSION = @SEQ66_API_VERSION@

SEQ66_LT_CURRENT = @SEQ66_LT_CURRENT@
SEQ66_LT_REVISION = @SEQ66_LT_REVISION@
SEQ66_LT_AGE = @SEQ66_LT_AGE@
SEQ66_LIBTOOL_VERSION = @SEQ66_LIBTOOL_VERSION@

QT_CXXFLAGS = @QT_CXXFLAGS@

QT_MOC = @QT_MOC@
QT_RCC = @QT_RCC@
QT_UIC = @QT_UIC@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
includedir = @seq66includedir@
libdir = @seq66libdir@
datadir = @datadir@
datarootdir = @datarootdir@
seq66includedir = @seq66includedir@
seq66libdir = @seq66libdir@

#******************************************************************************
# localedir
#------------------------------------------------------------------------------
#
# 	'localedir' is the normal system directory for installed localization
#  files.
#
#------------------------------------------------------------------------------

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#*****************************************************************************
# libtool
#-----------------------------------------------------------------------------

version = $(SEQ66_LIBTOOL_VERSION)

#*****************************************************************************
# git_version
#-----------------------------------------------------------------------------
# git_version = $(shell git describe --abbrev=7 --always --tags)
#-----------------------------------------------------------------------------

git_version = $(shell git describe --tags --long)
git_branch = $(shell git branch | grep -e ^*)
git_info = "$(git_version) $(git_branch)"

#*****************************************************************************
# Qt 5 results directories (avoid polluting the src directory)
#-----------------------------------------------------------------------------
#
#  The UI forms are stored in the "forms" directory, while the generated files
#  are stored in the "qt" directory, for both moc'ing and uic'ing.
#
# hppdir = ../include
#
#-----------------------------------------------------------------------------

thisdir = $(top_srcdir)/seq_qt5
incdir = $(top_srcdir)/seq_qt5/include
formsdir = $(top_srcdir)/seq_qt5/forms
resourcedir = $(top_srcdir)/resources
libseq66_incdir = $(top_srcdir)/libseq66/include
libsessions_incdir = $(top_srcdir)/libsessions/include

libseq66_libs = -L$(libseq66_libdir) -lseq66
libseq66_file = $(libseq66_libdir)/libseq66.la
libsessions_libs = -L$(libseq66_libdir) -lsessions
libsessions_file = $(libseq66_libdir)/libsessions.la
seq_rtmidi_incdir = $(top_srcdir)/seq_rtmidi/include
seq_portmidi_incdir = $(top_srcdir)/seq_portmidi/include

#******************************************************************************
# Compiler and linker flags (later, PortMidi)
#------------------------------------------------------------------------------
#
#	Unfortunately, we need to add the platform-specific include directories
#	because we include the performer module in some modules, and it includes
#	the platform-specific stuff.
#
# Notes:
#
#     -  None of ALSA or JACK CFLAGS seem to be defined.
#     -  We include "-I.." and require that the Qt5-generated modules be
#        included as:   "#include "qt/generated_module.h".
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(thisdir) \
 -I$(incdir) \
 -I$(formsdir) \
 -I$(libseq66_incdir) \
 -I$(libsessions_incdir) \
 -I$(seq_portmidi_incdir) \
 -I$(seq_rtmidi_incdir) \
 -I$(resourcedir) \
 $(ALSA_CFLAGS) \
 $(QT_CXXFLAGS) \
 $(JACK_CFLAGS) \
 -DSEQ66_GIT_VERSION=\"$(git_info)\"

#******************************************************************************
# Moc flags
#------------------------------------------------------------------------------

MOC_CPPFLAGS = -I$(top_srcdir)/include

if BUILD_QTMIDI
   MOC_CPPFLAGS += -DQ_OS_MAC
endif

#******************************************************************************
# Qt 5 rules
#------------------------------------------------------------------------------
#
#     $@ is the name of the file being generated;
#     $< is the first prerequisite (the source file)
#
#------------------------------------------------------------------------------

SUFFIXES = .h .hpp .cpp .moc.cpp .qrc .qrc.cpp .ui .ui.h

# The moc.cpp files are normally deposited in the include directory!
#
#     $(QT_MOC) $(MOC_CPPFLAGS) -o ../include/$(notdir $@) $<

%.moc.cpp: %.hpp
	$(QT_MOC) $(MOC_CPPFLAGS) -o $@ $<

# The ui.h files are deposited in the forms directory.

%.ui.h: %.ui
	$(QT_UIC) -o $@ $<

# The qrc files hold resources needed that aren't code.

%.qrc.cpp: %.qrc
	$(QT_RCC) -o $@ $<

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libseq_qt5.la

#******************************************************************************
# Source files
#----------------------------------------------------------------------------
#
#  We don't want to put Qt5-generated files into the "include" or "src"
#  directory; we want them in the "qt5" directory.
#
# moc_sources = $(moc_qt_headers:.hpp=.moc.cpp)
#
#  However....
#
#  Added qperfnames, qperfroll, qperftime, qseqdata, qseqkeys, qsmaintime,
#  qseqroll, qseqtime, qstriggereditor, due to vtable error for their
#  constructors or destructors.
#
#  Also note that we have to include ALL modules that declare QOBJECT in this
#  list, otherwise we get "undefined reference to 'vtable...'" for
#  their constructors.  Took awhile to figure that one out; qtcreator/qmake
#  took care of that issue, but in automake we have to do it ourselves.
#
#----------------------------------------------------------------------------

moc_qt_headers = \
 ../include/qclocklayout.hpp \
 ../include/qinputcheckbox.hpp \
 ../include/qlfoframe.hpp \
 ../include/qliveframeex.hpp \
 ../include/qloopbutton.hpp \
 ../include/qmutemaster.hpp \
 ../include/qpatternfix.hpp \
 ../include/qperfeditex.hpp \
 ../include/qperfeditframe64.hpp \
 ../include/qperfnames.hpp \
 ../include/qperfroll.hpp \
 ../include/qperftime.hpp \
 ../include/qplaylistframe.hpp \
 ../include/qportwidget.hpp \
 ../include/qsabout.hpp \
 ../include/qsbuildinfo.hpp \
 ../include/qseditoptions.hpp \
 ../include/qseqdata.hpp \
 ../include/qseqeditex.hpp \
 ../include/qseqeditframe64.hpp \
 ../include/qseqeventframe.hpp \
 ../include/qseqframe.hpp \
 ../include/qseqkeys.hpp \
 ../include/qseqroll.hpp \
 ../include/qseqtime.hpp \
 ../include/qsessionframe.hpp \
 ../include/qsetmaster.hpp \
 ../include/qseventslots.hpp \
 ../include/qslivegrid.hpp \
 ../include/qsmaintime.hpp \
 ../include/qsmainwnd.hpp \
 ../include/qstriggereditor.hpp \
 ../include/qt5nsmanager.hpp

moc_sources = $(moc_qt_headers:.hpp=.moc.cpp)

uic_qt_forms = \
 $(formsdir)/qlfoframe.ui \
 $(formsdir)/qliveframeex.ui \
 $(formsdir)/qmutemaster.ui \
 $(formsdir)/qpatternfix.ui \
 $(formsdir)/qperfeditex.ui \
 $(formsdir)/qperfeditframe64.ui \
 $(formsdir)/qplaylistframe.ui \
 $(formsdir)/qsabout.ui \
 $(formsdir)/qsbuildinfo.ui \
 $(formsdir)/qseditoptions.ui \
 $(formsdir)/qseqeditex.ui \
 $(formsdir)/qseqeditframe64.ui \
 $(formsdir)/qseqeventframe.ui \
 $(formsdir)/qsessionframe.ui \
 $(formsdir)/qsetmaster.ui \
 $(formsdir)/qslivegrid.ui \
 $(formsdir)/qsmainwnd.ui

uic_sources = $(uic_qt_forms:.ui=.ui.h)

BUILT_SOURCES = $(uic_sources) $(moc_sources)

#******************************************************************************
# BUILT_SOURCES
#------------------------------------------------------------------------------

libseq_qt5_la_SOURCES = \
 gui_palette_qt5.cpp \
 palettefile.cpp \
 qbase.cpp \
 qclocklayout.cpp \
 qeditbase.cpp \
 qinputcheckbox.cpp \
 qlfoframe.cpp \
 qliveframeex.cpp \
 qloopbutton.cpp \
 qmutemaster.cpp \
 qpatternfix.cpp \
 qperfbase.cpp \
 qperfeditex.cpp \
 qperfeditframe64.cpp \
 qperfnames.cpp \
 qperfroll.cpp \
 qperftime.cpp \
 qplaylistframe.cpp \
 qportwidget.cpp \
 qsabout.cpp \
 qsbuildinfo.cpp \
 qscrollmaster.cpp \
 qseditoptions.cpp \
 qseqbase.cpp \
 qseqdata.cpp \
 qseqeditex.cpp \
 qseqeditframe64.cpp \
 qseqeventframe.cpp \
 qseqframe.cpp \
 qseqkeys.cpp \
 qseqroll.cpp \
 qsessionframe.cpp \
 qseqtime.cpp \
 qsetmaster.cpp \
 qseventslots.cpp \
 qslivebase.cpp \
 qslivegrid.cpp \
 qslotbutton.cpp \
 qsmaintime.cpp \
 qsmainwnd.cpp \
 qstriggereditor.cpp \
 qt5_helpers.cpp \
 qt5nsmanager.cpp \
 $(moc_sources)

libseq_qt5_la_LDFLAGS = -version-info $(version)
libseq_qt5_la_LIBADD = $(ALSA_LIBS) $(JACK_LIBS)

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------
#
#     We'd like to remove /usr/local/include/libseq_qt5-1.0 if it is
#     empty.  However, we don't have a good way to do it yet.
#
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(libdir) manually"

#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------

