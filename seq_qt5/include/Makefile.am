#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_qt5 library
# \author     	Chris Ahlstrom
# \date       	2017-09-06
# \update      2020-12-21
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_qt5 C++
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

CLEANFILES = *.gc* *.moc.cpp
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

pkginclude_HEADERS = \
 gui_palette_qt5.hpp \
 palettefile.hpp \
 qbase.hpp \
 qclocklayout.hpp \
 qeditbase.hpp \
 qinputcheckbox.hpp \
 qlfoframe.hpp \
 qliveframeex.hpp \
 qloopbutton.hpp \
 qmutemaster.hpp \
 qperfbase.hpp \
 qperfeditex.hpp \
 qperfeditframe64.hpp \
 qperfnames.hpp \
 qperfroll.hpp \
 qperftime.hpp \
 qplaylistframe.hpp \
 qrollframe.hpp \
 qsabout.hpp \
 qsbuildinfo.hpp \
 qscrollmaster.h \
 qseditoptions.hpp \
 qseqbase.hpp \
 qseqdata.hpp \
 qseqeditex.hpp \
 qseqeditframe64.hpp \
 qseqeditframe.hpp \
 qseqeventframe.hpp \
 qseqframe.hpp \
 qseqkeys.hpp \
 qseqroll.hpp \
 qseqstyle.hpp \
 qseqtime.hpp \
 qsessionframe.hpp \
 qsetmaster.hpp \
 qskeymaps.hpp \
 qseventslots.hpp \
 qslivebase.hpp \
 qsliveframe.hpp \
 qslivegrid.hpp \
 qslotbutton.hpp \
 qsmacros.hpp \
 qsmaintime.hpp \
 qsmainwnd.hpp \
 qstriggereditor.hpp \
 qt5_helpers.hpp \
 qt5nsmanager.hpp 

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
