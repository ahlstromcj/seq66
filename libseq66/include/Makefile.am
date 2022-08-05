#******************************************************************************
# Makefile.am (libseq66)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq66 library
# \author     	Chris Ahlstrom
# \date       	2018-11-11
# \update      2022-08-05
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

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2 subdir-objects
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
# Source files
#----------------------------------------------------------------------------
#
#  We have replaced pkginclude_HEADERS with the nobase version in order to have
#  the header subdirectories created and used during "make install".  This
#  is necessary so that the #include statements are valid.
#
#----------------------------------------------------------------------------

nobase_include_HEADERS = \
 base64_images.hpp \
 seq66_features.h \
 seq66_features.hpp \
 seq66_platform_macros.h \
 cfg/basesettings.hpp \
 cfg/cmdlineopts.hpp \
 cfg/comments.hpp \
 cfg/configfile.hpp \
 cfg/midicontrolfile.hpp \
 cfg/mutegroupsfile.hpp \
 cfg/notemapfile.hpp \
 cfg/playlistfile.hpp \
 cfg/rcfile.hpp \
 cfg/rcsettings.hpp \
 cfg/recent.hpp \
 cfg/scales.hpp \
 cfg/sessionfile.hpp \
 cfg/settings.hpp \
 cfg/userinstrument.hpp \
 cfg/usermidibus.hpp \
 cfg/usrfile.hpp \
 cfg/usrsettings.hpp \
 ctrl/automation.hpp \
 ctrl/keycontrol.hpp \
 ctrl/keycontainer.hpp \
 ctrl/keymap.hpp \
 ctrl/keystroke.hpp \
 ctrl/midicontrolin.hpp \
 ctrl/midicontrol.hpp \
 ctrl/midicontrolbase.hpp \
 ctrl/midicontrolout.hpp \
 ctrl/midimacro.hpp \
 ctrl/midimacros.hpp \
 ctrl/midioperation.hpp \
 ctrl/opcontainer.hpp \
 ctrl/opcontrol.hpp \
 midi/businfo.hpp \
 midi/calculations.hpp \
 midi/controllers.hpp \
 midi/editable_event.hpp \
 midi/editable_events.hpp \
 midi/event.hpp \
 midi/eventlist.hpp \
 midi/jack_assistant.hpp \
 midi/mastermidibase.hpp \
 midi/mastermidibus.hpp \
 midi/midibase.hpp \
 midi/midibus_common.hpp \
 midi/midibus.hpp \
 midi/midibytes.hpp \
 midi/midifile.hpp \
 midi/midi_splitter.hpp \
 midi/midi_vector_base.hpp \
 midi/midi_vector.hpp \
 midi/wrkfile.hpp \
 play/clockslist.hpp \
 play/inputslist.hpp \
 play/metro.hpp \
 play/mutegroup.hpp \
 play/mutegroups.hpp \
 play/notemapper.hpp \
 play/performer.hpp \
 play/playlist.hpp \
 play/portslist.hpp \
 play/screenset.hpp \
 play/seq.hpp \
 play/sequence.hpp \
 play/setmapper.hpp \
 play/setmaster.hpp \
 play/songsummary.hpp \
 play/triggers.hpp \
 sessions/clinsmanager.hpp \
 sessions/smanager.hpp \
 os/daemonize.hpp \
 os/shellexecute.hpp \
 os/timing.hpp \
 util/automutex.hpp \
 util/basic_macros.h \
 util/basic_macros.hpp \
 util/condition.hpp \
 util/filefunctions.hpp \
 util/named_bools.hpp \
 util/palette.hpp \
 util/recmutex.hpp \
 util/rect.hpp \
 util/strfunctions.hpp

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (libseq66)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------

