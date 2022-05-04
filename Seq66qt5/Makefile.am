#******************************************************************************
# Makefile.am (seq66qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	seq66qt5 application
# \author     	Chris Ahlstrom
# \date       	2017-09-05
# \update      2022-05-04
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
libseq_qt5dir = $(builddir)/seq_qt5/src/.libs

#******************************************************************************
# AM_CPPFLAGS [formerly "INCLUDES"]
#------------------------------------------------------------------------------
#
# 	'AM_CPPFLAGS' is the set of directories needed to access all of the
# 	library header files used in this project.
#
# 	Note that we include both the portmidi and rtmidi include directories.
# 	There are no conflicts in names of header files, and we don't want to
# 	try to test SEQ66_RTMIDI_SUPPORT and SEQ66_PORTMIDI_SUPPORT until we have
# 	to.
#
#  -I$(top_srcdir)/libsessions/include
#
#------------------------------------------------------------------------------

if BUILD_RTMIDI
MIDIHDR = $(top_srcdir)/seq_rtmidi/include
MIDILIB = -L$(libseq_rtmididir) -lseq_rtmidi
MIDIDEP = $(libseq_rtmididir)/libseq_rtmidi.la
MIDIAPP = qseq66
else
MIDIHDR = $(top_srcdir)/seq_portmidi/include
MIDILIB = -L$(libseq_portmididir) -lseq_portmidi
MIDIDEP = $(libseq_portmididir)/libseq_portmidi.la
MIDIAPP = qpseq66
endif

AM_CXXFLAGS = \
 -I$(top_srcdir)/libseq66/include \
 -I$(top_srcdir)/libsessions/include \
 -I$(MIDIHDR) \
 -I$(top_srcdir)/seq_qt5/include \
 $(QT_CXXFLAGS) \
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
 $(MIDILIB)

#****************************************************************************
# Project-specific dependency files
#----------------------------------------------------------------------------
#
#  Provides the specific list of dependencies, to assure that the make
#  detects all changes, if they are available.
#
#----------------------------------------------------------------------------

dependencies = \
 $(MIDIDEP) \
 $(libseq_qt5dir)/libseq_qt5.la \
 $(libsessionsdir)/libsessions.la \
 $(libseq66dir)/libseq66.la

#******************************************************************************
# The programs to build
#------------------------------------------------------------------------------

bin_PROGRAMS = $(MIDIAPP)

#******************************************************************************
# seq66
#----------------------------------------------------------------------------
#
#  Added a LDFLAGS line to avoid a "DSO missing" error on some systems when
#  building in debug mode:
#
#     As a workaround it's possible to switch back to the more permissive
#     view of what symbols are available by using the option
#
#        -Wl,--copy-dt-needed-entries
#
#  See this article:
#
#  https://flameeyes.blog/2010/11/26/
#     it-s-not-all-gold-that-shines-why-underlinking-is-a-bad-thing/
#
#----------------------------------------------------------------------------
 
qseq66_SOURCES = seq66qt5.cpp
qseq66_DEPENDENCIES = $(NSM_DEPS) $(dependencies)
qseq66_LDFLAGS = -Wl,--copy-dt-needed-entries
qseq66_LDADD = $(NSM_LIBS) $(libraries) $(LIBLO_LIBS) $(QT_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(AM_LDFLAGS)
 
qpseq66_SOURCES = seq66qt5.cpp
qpseq66_DEPENDENCIES = $(NSM_DEPS) $(dependencies)
qpseq66_LDFLAGS = -Wl,--copy-dt-needed-entries
qpseq66_LDADD = $(NSM_LIBS) $(libraries) $(LIBLO_LIBS) $(QT_LIBS) $(ALSA_LIBS) $(JACK_LIBS) $(AM_LDFLAGS)

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
