#******************************************************************************
# Makefile.am (seq66cli)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	seq66cli application
# \author     	Chris Ahlstrom
# \date       	2017-04-07
# \update      2020-11-11
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the seq66cli C/C++
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
sequencer66includedir = @sequencer66includedir@
sequencer66libdir = @sequencer66libdir@

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
libseq_portmididir = $(builddir)/seq_portmidi/src/.libs

#******************************************************************************
# AM_CPPFLAGS [formerly "INCLUDES"]
#------------------------------------------------------------------------------
#
# 	'AM_CPPFLAGS' is the set of directories needed to access all of the
# 	library header files used in this project.
#
#   -I$(top_srcdir)/seq_gtkmm2/include \
#
#------------------------------------------------------------------------------

if BUILD_WINDOWS
AM_CXXFLAGS = \
 -I$(top_srcdir)/libseq66/include \
 -I$(top_srcdir)/libsessions/include \
 -I$(top_srcdir)/seq_portmidi/include \
 $(JACK_CFLAGS) \
 $(LIBLO_CFLAGS) \
 $(NSM_CFLAGS) \
 $(LASH_CFLAGS)
else
AM_CXXFLAGS = \
 -I$(top_srcdir)/libseq66/include \
 -I$(top_srcdir)/libsessions/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 $(JACK_CFLAGS) \
 $(LIBLO_CFLAGS) \
 $(NSM_CFLAGS) \
 $(LASH_CFLAGS)
endif

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
#  $(libmath)
#  -L$(libseq_gtkmm2dir) -lseq_gtkmm2 \
#
#----------------------------------------------------------------------------

if BUILD_WINDOWS
libraries = \
 -L$(libsessionsdir) -lsessions \
 -L$(libseq66dir) -lseq66 \
 -L$(libseq_portmididir) -lseq_portmidi \
 -L$(libsessionsdir) -lsessions \
 -lwinmm
else
libraries = \
 -L$(libsessionsdir) -lsessions \
 -L$(libseq66dir) -lseq66 \
 -L$(libsessionsdir) -lsessions \
 -L$(libseq_rtmididir) -lseq_rtmidi
endif

#****************************************************************************
# Project-specific dependency files
#----------------------------------------------------------------------------
#
#  Provdies the specific list of dependencies, to assure that the make
#  detects all changes, if they are available.
#
#  $(libseq_gtkmm2dir)/libseq_gtkmm2.la \
#----------------------------------------------------------------------------

if BUILD_WINDOWS
dependencies = \
 $(libseq_portmididir)/libseq_portmidi.la \
 $(libsessionsdir)/libsessions.la \
 $(libseq66dir)/libseq66.la
else
dependencies = \
 $(libseq_rtmididir)/libseq_rtmidi.la \
 $(libsessionsdir)/libsessions.la \
 $(libseq66dir)/libseq66.la
endif

#******************************************************************************
# The programs to build
#------------------------------------------------------------------------------

bin_PROGRAMS = seq66cli

#******************************************************************************
# seq66cli
#----------------------------------------------------------------------------

seq66cli_SOURCES = seq66rtcli.cpp
seq66cli_DEPENDENCIES = $(NSM_DEPS) $(dependencies)

if BUILD_WINDOWS
seq66cli_LDADD = $(NSM_LIBS) $(libraries) $(LIBLO_LIBS) $(AM_LDFLAGS) $(PTHREAD_LIBS)
else
seq66cli_LDADD = $(NSM_LIBS) $(libraries) $(LIBLO_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS) $(AM_LDFLAGS)
seq66cli_LDFLAGS = -Wl,--copy-dt-needed-entries
endif

#******************************************************************************
# Testing
#------------------------------------------------------------------------------
#
# 	   http://www.gnu.org/software/hello/manual/automake/Simple-Tests.html
#
#     Not supported for this application.  We supply a bogus test script.
#
#------------------------------------------------------------------------------

# .PHONY: test_script
# 
# TESTS = test_script
# 
# test: check

#******************************************************************************
#  distclean
#------------------------------------------------------------------------------

distclean-local:
	-rm -rf $(testsubdir)

#******************************************************************************
# Makefile.am (seq66cli)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
