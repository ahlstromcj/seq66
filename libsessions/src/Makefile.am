#******************************************************************************
# Makefile.am (libsessions/src)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libsessions library
# \author     	Chris Ahlstrom
# \date       	2020-03-08
# \update      2020-08-20
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libsessions C/C++
# 		library.
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

version = $(SEQ66_API_MAJOR):$(SEQ66_API_MINOR):$(SEQ66_API_PATCH)

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
#  -I$(top_srcdir)/seq_gtkmm2/include
#
#	Unfortunately, we need to add the platform-specific include directories
#	because we include the performer module in some modules, and it includes
#	the platform-specific stuff.
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/libseq66/include \
 -I$(top_srcdir)/libsessions/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 -I$(top_srcdir)/seq_portmidi/include \
 $(ALSA_CFLAGS) \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -DSEQ66_GIT_VERSION=\"$(git_info)\"

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libsessions.la

#******************************************************************************
# Source files
#----------------------------------------------------------------------------

libsessions_la_SOURCES = \
 lash/lash.cpp \
 nsm/nsmbase.cpp \
 nsm/nsmclient.cpp \
 nsm/nsmmessages.cpp \
 nsm/nsmserver.cpp

libsessions_la_LDFLAGS = -version-info $(version)
libsessions_la_LIBADD = $(ALSA_LIBS) $(JACK_LIBS) $(LASH_LIBS)

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
# Makefile.am (libsessions/src)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------

