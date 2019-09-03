#****************************************************************************
# Makefile.am (seq66-tests)
#----------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	Seqtool test
# \author     	Chris Ahlstrom
# \date       	2018-11-11
# \update       2018-02-10
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
#		This is a Makefile for the tests directory for the Seqtool test
#		project.  This module provides an automake makefile for the unit-test,
#		developer tests, or integration test applications.  This makefile
#		provides the items needed to build the test applications using GNU
#		autotools.
#
#----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2 subdir-objects
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#******************************************************************************
# CLEANFILES
#------------------------------------------------------------------------------
#
#  Do not forget to clean the moc/uic/rcc generated files.
#
#------------------------------------------------------------------------------

CLEANFILES = *.gc* *.moc.cpp *.moc.o ../forms/*.ui.h qseq66.qrc.cpp
MOSTLYCLEANFILES = *~

#****************************************************************************
# 	EXTRA_DIST
#----------------------------------------------------------------------------
#
#	Add any extra distributable files here, so they are added to the tar
#	package.
#
# EXTRA_DIST = test_script
#
#----------------------------------------------------------------------------

EXTRA_DIST =

#******************************************************************************
# Packaging items from configure.ac
#-------------------------------------------------------------------------------
#
#  XPCCUT_LIBS is defined by the PKG_CHECK_MODULES macro.  The xpccut
#  library must be installed officially in order to be used to build the
#  unit-test application of the xpc library.
#
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

# XPCCUTPP_CFLAGS = @XPCCUTPP_CFLAGS@
# XPCCUT_CFLAGS = @XPCCUT_CFLAGS@

XPCCUTPP_LIBS = @XPCCUTPP_LIBS@
XPCCUT_LIBS = @XPCCUT_LIBS@

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
# git_version
#-----------------------------------------------------------------------------
# git_version = $(shell git describe --abbrev=7 --always --tags)
#-----------------------------------------------------------------------------

git_version = $(shell git describe --tags --long)
git_branch = $(shell git branch | grep -e ^*)
git_info = "$(git_version) $(git_branch)"

#******************************************************************************
# Locations of the test libraries and header files
#------------------------------------------------------------------------------
#
#		It is assumed that the seq66 project and this test project have
#		their roots at the same level (e.g. both are in the "git"
#		directory).  They are related projects, but we don't want to force
#		the user/developer to download and build the unit-test libraries
#		just to be able to build seq66.
#
#------------------------------------------------------------------------------

formsdir = ../forms

libseq66_incdir = $(top_srcdir)/libseq66/include
libseq66_cfgdir = $(top_srcdir)/libseq66/include/cfg
libseq66_ctrldir = $(top_srcdir)/libseq66/include/ctrl
libseq66_utildir = $(top_srcdir)/libseq66/include/util
libseq66_libdir = $(top_srcdir)/libseq66/src       # .libs/
libseq66_libs = -L$(libseq66_libdir) -lseq66
libseq66_file = $(libseq66_libdir)/libseq66.la
seq_rtmidi_incdir = $(top_srcdir)/seq_rtmidi/include
seq_rtmidi_libdir = $(top_srcdir)/seq_rtmidi/src       # .libs/
seq_rtmidi_libs = -L$(seq_rtmidi_libdir) -lseq_rtmidi
seq_portmidi_incdir = $(top_srcdir)/seq_portmidi/include
seq_portmidi_libdir = $(top_srcdir)/seq_portmidi/src       # .libs/
seq_portmidi_libs = -L$(seq_portmidi_libdir) -lseq_portmidi

.PHONY: $(libseq66_file)

dependencies = $(libseq66_file) # $(XPCCUTPP_LIBS) $(XPCCUT_LIBS)

#*****************************************************************************
# libtool
#-----------------------------------------------------------------------------
#
#     GNU automake assigns, for us: LIBTOOL_DEPS = @LIBTOOL_DEPS@
#
# libtool: $(LIBTOOL_DEPS)
#    $(SHELL) ./scripts/config.status --recheck
#
#-----------------------------------------------------------------------------

version = $(XPC_API_MAJOR):$(XPC_API_MINOR):$(XPC_API_PATCH)

#****************************************************************************
# AM_CPPFLAGS [formerly "INCLUDES"]
#----------------------------------------------------------------------------
#
#	Specifies the header-file locations for uninstalled libraries in the XPC
#	source tree.
#
#   Hmmm, the subdirectories are already specified when #include'ing the
#   header files.
#
#----------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I.. \
 -I../include \
 -I$(libseq66_incdir) \
 -I$(libseq66_cfgdir) \
 -I$(libseq66_ctrldir) \
 -I$(libseq66_utildir) \
 -I$(seq_portmidi_incdir) \
 -I$(seq_rtmidi_incdir) \
 $(ALSA_CFLAGS) \
 $(QT_CXXFLAGS) \
 $(JACK_CFLAGS) \
 $(XPCCUTPP_CFLAGS) \
 $(XPCCUT_CFLAGS) \
 -DSEQ66_GIT_VERSION=\"$(git_info)\"

#******************************************************************************
# Moc flags
#------------------------------------------------------------------------------

MOC_CPPFLAGS = -I$(top_srcdir)/include

if BUILD_QTMIDI
   MOC_CPPFLAGS += -DQ_OS_MAC
endif

if BUILD_WINDOWS
   MOC_CPPFLAGS += -D_WIN32
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

#****************************************************************************
# The program(s) to build, but not install
#----------------------------------------------------------------------------

bin_PROGRAMS = seqtool

#******************************************************************************
# Source files
#----------------------------------------------------------------------------

moc_qt_headers = ../include/qtcore_task.hpp ../include/qtestframe.hpp

moc_sources = $(moc_qt_headers:.hpp=.moc.cpp)

uic_qt_forms = $(formsdir)/qtestframe.ui

uic_sources = $(uic_qt_forms:.ui=.ui.h)

BUILT_SOURCES = $(uic_sources) $(moc_sources)

#****************************************************************************
# SOURCES
#----------------------------------------------------------------------------
#
#  Note that we assume the developer has also downloaded and built, in this
#  same directory tree, the C library, xpccut.  If a bug is found, the
#  developer will need both sets of source code anyway.
#
#----------------------------------------------------------------------------

seqtool_SOURCES = \
 converter.cpp \
 faker.cpp \
 gdk_basic_keys.cpp \
 midi_control_helpers.cpp \
 midi_control_unit_test.cpp \
 optionsfile.cpp \
 qtcore_task.cpp \
 qtestframe.cpp \
 seqtool.cpp \
 unit_tests.cpp \
 util_unit_test.cpp \
 $(moc_sources)

# -lpthread $(libraries)
#
# _LIBADD or _LDADD?
#
# seqtool_LDFLAGS = -version-info $(version)

seqtool_LDADD = \
 $(libseq66_libs) \
 $(seq_rtmidi_libs) \
 $(QT_LIBS) \
 $(XPCCUTPP_LIBS) \
 $(XPCCUT_LIBS) \
 $(ALSA_LIBS) \
 $(JACK_LIBS)

seqtool_DEPENDENCIES = $(dependencies)

#****************************************************************************
# TESTS
#----------------------------------------------------------------------------
#
#     This variable is used in the "make check" command.  We add a target to
#     implement "make test", also.
#
#----------------------------------------------------------------------------

# testsubdir = test-results
# TESTS_ENVIRONMENT =
# TESTS = test_script
# test: check

#****************************************************************************
# distclean
#----------------------------------------------------------------------------

distclean-local:
	-rm -rf $(testsubdir)

#****************************************************************************
# Makefile.am (Seqtool)
#----------------------------------------------------------------------------
# vim: ts=4 sw=4 wm=8 ft=automake
#----------------------------------------------------------------------------
