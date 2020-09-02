#******************************************************************************
# Makefile.am (seq66qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	seq66qt5 application
# \author     	Chris Ahlstrom
# \date       	2017-09-05
# \update      2020-09-02
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the seq66qt5 C/C++
# 		application.
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#******************************************************************************
# CLEANFILES
#------------------------------------------------------------------------------

CLEANFILES = *.gc*

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------
#
#  getopt_test.c is not ready and is not included at this time.
#	$(TESTS) is not included because it is derived files.
#
#------------------------------------------------------------------------------

# EXTRA_DIST = dl_leaks.supp make-tests README

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------
#
# 	Not needed, yet, since we won't be installing the app for awhile.
#
#------------------------------------------------------------------------------

prefix = @prefix@
libdir = @xpclibdir@
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

libseq66dir = $(builddir)/libseq66/src/.libs
libsessionsdir = $(builddir)/libsessions/src/.libs
libseq_rtmididir = $(builddir)/seq_rtmidi/src/.libs
libseq_qt5dir = $(builddir)/seq_qt5/src/.libs

#******************************************************************************
# AM_CPPFLAGS [formerly "INCLUDES"]
#------------------------------------------------------------------------------
#
# 	'AM_CPPFLAGS' is the set of directories needed to access all of the
# 	library header files used in this project.
#
# 	Note that we include both the portmidi and rtmidi include directories.
# 	There are no conflicts in name of header files, and we don't want to
# 	try to test SEQ66_RTMIDI_SUPPORT and SEQ66_PORTMIDI_SUPPORT until we have
# 	to.
#
#  -I$(top_srcdir)/libsessions/include
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I$(top_srcdir)/libseq66/include \
 -I$(top_srcdir)/libsessions/include \
 -I$(top_srcdir)/seq_portmidi/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 -I$(top_srcdir)/seq_qt5/include \
 $(QT_CXXFLAGS) \
 $(JACK_CFLAGS) \
 $(LIBLO_CFLAGS) \
 $(NSM_CFLAGS) \
 $(LASH_CFLAGS)

#******************************************************************************
# libmath
#------------------------------------------------------------------------------
#
# 		One day, we got errors about sqrt() undefined, which we fixed by
# 		adding -lm.  Then one day we got errors about various items in
# 		sys/stat.h being multiply-defined, and it turned out to be the -lm.
#
# 		We make it (an empty) define for how to handle it more easily.
#
#------------------------------------------------------------------------------

libmath = -lm

#****************************************************************************
# Project-specific library files
#----------------------------------------------------------------------------
#
#	These files are the ones built in the source tree, not the installed
#	ones.
#
#  Sometimes one has to change the order of the libraries in this list.
#
# $(libmath)
#
#  The libsessions library is handled via the NSM_LIBS macro, since some users
#  might not want to build in Non Session Manager support.
#
#----------------------------------------------------------------------------

libraries = \
 -L$(libseq_qt5dir) -lseq_qt5 \
 -L$(libsessionsdir) -lsessions \
 -L$(libseq66dir) -lseq66 \
 -L$(libsessionsdir) -lsessions \
 -L$(libseq_rtmididir) -lseq_rtmidi

#****************************************************************************
# Project-specific dependency files
#----------------------------------------------------------------------------
#
#  Provides the specific list of dependencies, to assure that the make
#  detects all changes, if they are available.
#
#----------------------------------------------------------------------------

dependencies = \
 $(libseq_rtmididir)/libseq_rtmidi.la \
 $(libseq_qt5dir)/libseq_qt5.la \
 $(libsessionsdir)/libsessions.la \
 $(libseq66dir)/libseq66.la

#******************************************************************************
# The programs to build
#------------------------------------------------------------------------------

bin_PROGRAMS = qseq66

#******************************************************************************
# seq66
#----------------------------------------------------------------------------
# qseq66_LDFLAGS = -Wl,--start-group $(libraries) -Wl,--end-group
#----------------------------------------------------------------------------
 
qseq66_SOURCES = seq66qt5.cpp
qseq66_DEPENDENCIES = $(NSM_DEPS) $(dependencies)
qseq66_LDADD = $(NSM_LIBS) $(libraries) $(LIBLO_LIBS) $(QT_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS) $(AM_LDFLAGS)

#******************************************************************************
# Testing
#------------------------------------------------------------------------------
#
# 	   http://www.gnu.org/software/hello/manual/automake/Simple-Tests.html
#
#     Not supported for this application.  We supply a bogus test script.
#
#------------------------------------------------------------------------------

.PHONY: test_script

TESTS = test_script
 
test: check

#******************************************************************************
#  distclean
#------------------------------------------------------------------------------

distclean-local:
	-rm -rf $(testsubdir)

#******************************************************************************
# Makefile.am (seq66qt5)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
