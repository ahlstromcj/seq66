#******************************************************************************
# Makefile.am (libseq66/src)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq66 library
# \author     	Chris Ahlstrom
# \date       	2018-11-11
# \update      2020-04-15
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq66 C/C++
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
#	Unfortunately, we need to add the platform-specific include directories
#	because we include the performer module in some modules, and it includes
#	the platform-specific stuff.
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/libseq66/include \
 -I$(top_srcdir)/seq_portmidi/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 $(ALSA_CFLAGS) \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -DSEQ66_GIT_VERSION=\"$(git_info)\"

#******************************************************************************
# The library to build, a libtool-based library
#------------------------------------------------------------------------------

lib_LTLIBRARIES = libseq66.la

#******************************************************************************
# Source files
#----------------------------------------------------------------------------

libseq66_la_SOURCES = \
 seq66_features.cpp \
 cfg/basesettings.cpp \
 cfg/cmdlineopts.cpp \
 cfg/comments.cpp \
 cfg/configfile.cpp \
 cfg/midicontrolfile.cpp \
 cfg/mutegroupsfile.cpp \
 cfg/notemapfile.cpp \
 cfg/rcfile.cpp \
 cfg/rcsettings.cpp \
 cfg/recent.cpp \
 cfg/scales.cpp \
 cfg/settings.cpp \
 cfg/userinstrument.cpp \
 cfg/usermidibus.cpp \
 cfg/usrfile.cpp \
 cfg/usrsettings.cpp \
 ctrl/automation.cpp \
 ctrl/keycontainer.cpp \
 ctrl/keycontrol.cpp \
 ctrl/keymap.cpp \
 ctrl/keystroke.cpp \
 ctrl/midicontrolin.cpp \
 ctrl/midicontrolbase.cpp \
 ctrl/midicontrol.cpp \
 ctrl/midicontrolout.cpp \
 ctrl/midioperation.cpp \
 ctrl/opcontainer.cpp \
 ctrl/opcontrol.cpp \
 midi/businfo.cpp \
 midi/controllers.cpp \
 midi/editable_event.cpp \
 midi/editable_events.cpp \
 midi/event.cpp \
 midi/eventlist.cpp \
 midi/jack_assistant.cpp \
 midi/mastermidibase.cpp \
 midi/midibase.cpp \
 midi/midibytes.cpp \
 midi/midifile.cpp \
 midi/midi_splitter.cpp \
 midi/midi_vector_base.cpp \
 midi/midi_vector.cpp \
 midi/wrkfile.cpp \
 play/mutegroup.cpp \
 play/mutegroups.cpp \
 play/notemapper.cpp \
 play/performer.cpp \
 play/playlist.cpp \
 play/screenset.cpp \
 play/seq.cpp \
 play/sequence.cpp \
 play/setmapper.cpp \
 play/triggers.cpp \
 sessions/smanager.cpp \
 sessions/smfunctions.cpp \
 unix/daemonize.cpp \
 util/automutex.cpp \
 util/basic_macros.cpp \
 util/calculations.cpp \
 util/condition.cpp \
 util/filefunctions.cpp \
 util/palette.cpp \
 util/recmutex.cpp \
 util/rect.cpp \
 util/strfunctions.cpp

libseq66_la_LDFLAGS = -version-info $(version)
libseq66_la_LIBADD = $(ALSA_LIBS) $(JACK_LIBS)

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
# Makefile.am (libseq66/src)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------

