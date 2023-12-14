#******************************************************************************
# Makefile.am (seq66cli)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	seq66cli application
# \author     	Chris Ahlstrom
# \date       	2017-04-07
# \update      2023-12-13
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

if BUILD_RTMIDI
MIDIHDR = $(top_srcdir)/seq_rtmidi/include
MIDILIB = -L$(libseq_rtmididir) -lseq_rtmidi
MIDIDEP = $(libseq_rtmididir)/libseq_rtmidi.la
MIDIAPP = seq66cli
else
MIDIHDR = $(top_srcdir)/seq_portmidi/include
MIDILIB = -L$(libseq_portmididir) -lseq_portmidi
MIDIDEP = $(libseq_portmididir)/libseq_portmidi.la
MIDIAPP = seq66clip
endif

AM_CXXFLAGS = \
 -I$(top_srcdir)/libseq66/include \
 -I$(top_srcdir)/libsessions/include \
 -I$(MIDIHDR) \
 $(JACK_CFLAGS) \
 $(LIBLO_CFLAGS) \
 $(NSM_CFLAGS)

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

libraries = \
 -L$(libsessionsdir) -lsessions \
 -L$(libseq66dir) -lseq66 \
 -L$(libsessionsdir) -lsessions \
 $(MIDILIB)

#****************************************************************************
# Project-specific dependency files
#----------------------------------------------------------------------------
#
#  Provdies the specific list of dependencies, to assure that the make
#  detects all changes, if they are available.
#
#  $(libseq_gtkmm2dir)/libseq_gtkmm2.la \
#----------------------------------------------------------------------------

dependencies = \
 $(MIDIDEP) \
 $(libsessionsdir)/libsessions.la \
 $(libseq66dir)/libseq66.la

#******************************************************************************
# The programs to build
#------------------------------------------------------------------------------

bin_PROGRAMS = $(MIDIAPP)

#******************************************************************************
# seq66cli
#
# seq66cli_LDFLAGS = -Wl,--copy-dt-needed-entries
#
#----------------------------------------------------------------------------

seq66cli_SOURCES = seq66rtcli.cpp
seq66cli_DEPENDENCIES = $(NSM_DEPS) $(dependencies)
seq66cli_LDFLAGS = $(LINKER_FLAGS_ADD)
seq66cli_LDADD = $(NSM_LIBS) $(libraries) $(LIBLO_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(AM_LDFLAGS)

seq66clip_SOURCES = seq66rtcli.cpp
seq66clip_DEPENDENCIES = $(NSM_DEPS) $(dependencies)
seq66clip_LDFLAGS = $(LINKER_FLAGS_ADD)
seq66clip_LDADD = $(NSM_LIBS) $(libraries) $(LIBLO_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(AM_LDFLAGS)

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
