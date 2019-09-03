#******************************************************************************
# Makefile.am (seq66-tests)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	Seqtool library
# \author     	Chris Ahlstrom
# \date       	2018-11-11
# \update       2018-12-22
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the sequencer26 C/C++
# 		tests.
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

EXTRA_DIST = *.hpp

#******************************************************************************
# Items from configure.ac
#-------------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

#******************************************************************************
# Source files
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
 converter.hpp \
 faker.hpp \
 gdk_basic_keys.hpp \
 midi_control_helpers.hpp \
 midi_control_unit_test.hpp \
 optionsfile.hpp \
 qtcore_task.hpp \
 qtestframe.hpp \
 unit_tests.hpp \
 util_unit_test.hpp

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (seq66-tests)
#------------------------------------------------------------------------------
# 	vim: ts=4 sw=4 ft=automake
#------------------------------------------------------------------------------
