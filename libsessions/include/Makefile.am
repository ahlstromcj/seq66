#******************************************************************************
# Makefile.am (libseq66)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq66 library
# \author     	Chris Ahlstrom
# \date       	2020-03-08
# \update      2020-09-01
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
# Compiler and linker flags
#------------------------------------------------------------------------------
#
#  $(GTKMM_CFLAGS)
#
#------------------------------------------------------------------------------

AM_CXXFLAGS = \
 -I../include \
 -I$(top_srcdir)/include \
 $(JACK_CFLAGS) \
 $(LASH_CFLAGS) \
 -DSEQ66_GIT_VERSION=\"$(git_info)\" \
 -Wall $(MM_WFLAGS)

#******************************************************************************
# Source files
#----------------------------------------------------------------------------
#
#  Not added: nsm.h, nsmdummy.h.
#
#----------------------------------------------------------------------------

pkginclude_HEADERS = \
 lash/lash.hpp \
 nsm/nsmbase.hpp \
 nsm/nsmclient.hpp \
 nsm/nsmmessagesex.hpp \
 nsm/nsmserver.hpp

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

