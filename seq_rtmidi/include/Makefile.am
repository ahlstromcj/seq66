#******************************************************************************
# Makefile.am (seq_rtmidi)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	seq_rtmidi library
# \author     	Chris Ahlstrom
# \date       	2016-11-19
# \update      2017-02-10
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the seq66 C/C++
# 		library.
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)
CLEANFILES = *.gc*
MOSTLYCLEANFILES = *~

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = *.h *.hpp

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@
GIT_VERSION = @GIT_VERSION@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

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
# Compiler and linker flags
#------------------------------------------------------------------------------
#
#  $(GTKMM_CFLAGS)
#  $(JACK_CFLAGS)
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 -I$(top_srcdir)/seq_rtmidi/include \
 -DSEQ66_GIT_VERSION=\"$(git_info)\" \
 -Wall $(MM_WFLAGS)

#******************************************************************************
# Source files
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
   mastermidibus_rm.hpp \
   midibus_rm.hpp \
	midi_alsa.hpp \
	midi_alsa_info.hpp \
	midi_api.hpp \
	midi_info.hpp \
	midi_jack.hpp \
	midi_jack_data.hpp \
	midi_jack_info.hpp \
	midi_probe.hpp \
	rterror.hpp \
	rtmidi.hpp \
	rtmidi_info.hpp \
	seq66_rtmidi_features.h

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (seq_rtmidi)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
