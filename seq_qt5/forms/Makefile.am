#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
##
# \file         Makefile.am
# \library      libseq_qt5 library
# \author       Chris Ahlstrom
# \date         2018-03-09
# \update       2023-08-21
# \version      $Revision$
# \license      $XPC_SUITE_GPL_LICENSE$
#
#       This module provides an Automake makefile for the libseq_qt5 C++
#       library.
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

CLEANFILES = *.ui.h
MOSTLYCLEANFILES = *~

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = *.ui

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

UI_FILES = \
 qlfoframe.ui \
 qliveframeex.ui \
 qmutemaster.ui \
 qperfeditframe64.ui \
 qsabout.ui \
 qsappinfo.ui \
 qsbuildinfo.ui \
 qseditoptions.ui \
 qseqeditex.ui \
 qseqeditframe64.ui \
 qseqeventframe.ui \
 qsetmaster.ui \
 qslivegrid.ui \
 qsmainwnd.ui

UI_H_FILES = \
 qlfoframe.ui.h \
 qliveframeex.ui.h \
 qmutemaster.ui.h \
 qperfeditframe64.ui.h \
 qsabout.ui.h \
 qsappinfo.ui.h \
 qsbuildinfo.ui.h \
 qseditoptions.ui.h \
 qseqeditex.ui.h \
 qseqeditframe64.ui.h \
 qseqeventframe.ui.h \
 qsetmaster.ui.h \
 qslivegrid.ui.h \
 qsmainwnd.ui.h

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	@echo "Note:  you may want to remove $(pkgincludedir) manually"

#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
