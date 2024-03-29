#******************************************************************************
# Makefile.am (seq_rtmidi/src)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	seq_rtmidi library
# \author     	Chris Ahlstrom
# \date       	2016-11-19
# \update      2022-09-13
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the seq_rtmidi C/C++
# 		library.
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
git_branch =$(shell git branch | grep -e ^*)
git_info = "$(git_version) $(git_branch)"

#******************************************************************************
# Compiler and linker flags
#
#  $(GTKMM_CFLAGS)
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/libseq66/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 $(ALSA_CFLAGS) \
 $(JACK_CFLAGS) \
 $(CALLFLAG) \
 -DSEQ66_GIT_VERSION=\"$(git_info)\"

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libseq_rtmidi.la

#******************************************************************************
# Source files
#
#  We include only the JACK and ALSA support here.
#
# midi_jack.cpp
#
#----------------------------------------------------------------------------

libseq_rtmidi_la_SOURCES = \
   mastermidibus.cpp \
   midibus.cpp \
	midi_alsa.cpp \
	midi_alsa_info.cpp \
	midi_api.cpp \
	midi_info.cpp \
	midi_jack.cpp \
	midi_jack_data.cpp \
	midi_jack_info.cpp \
	midi_probe.cpp \
	rtmidi.cpp \
	rtmidi_info.cpp \
	rtmidi_types.cpp

libseq_rtmidi_la_LDFLAGS = -version-info $(version)
libseq_rtmidi_la_LIBADD = $(ALSA_LIBS) $(JACK_LIBS)

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------
#
#     We'd like to remove /usr/local/include/seq66-1.0 if it is
#     empty.  However, we don't have a good way to do it yet.
#
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(libdir) manually"

#******************************************************************************
# Makefile.am (seq_rtmidi/src)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
