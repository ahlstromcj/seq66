#******************************************************************************
# Makefile.am (libseq_qt5)
#------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	libseq_qt5 library
# \author     	Chris Ahlstrom
# \date       	2017-09-06
# \update      2023-08-21
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		This module provides an Automake makefile for the libseq_qt5 C++
# 		library.
#
# Removed:
#
#     qseqeditframe.hpp
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)
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
 qpatternfix.hpp \
 qperfbase.hpp \
 qperfeditex.hpp \
 qperfeditframe64.hpp \
 qperfnames.hpp \
 qperfroll.hpp \
 qperftime.hpp \
 qplaylistframe.hpp \
 qportwidget.hpp \
 qsabout.hpp \
 qsappinfo.hpp \
 qsbuildinfo.hpp \
 qscrollmaster.h \
 qscrollslave.h \
 qseditoptions.hpp \
 qseqbase.hpp \
 qseqdata.hpp \
 qseqeditex.hpp \
 qseqeditframe64.hpp \
 qseqeventframe.hpp \
 qseqframe.hpp \
 qseqkeys.hpp \
 qseqroll.hpp \
 qseqtime.hpp \
 qsessionframe.hpp \
 qsetmaster.hpp \
 qseventslots.hpp \
 qslivebase.hpp \
 qslivegrid.hpp \
 qslotbutton.hpp \
 qsmaintime.hpp \
 qsmainwnd.hpp \
 qstriggereditor.hpp \
 qt5_helper.h \
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
