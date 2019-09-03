#*****************************************************************************
# Makefile.am (libseq66)
#-----------------------------------------------------------------------------
##
# \file          Makefile.am
# \library       libseq66
# \author        Chris Ahlstrom
# \date          2018-11-11
# \updates       2018-12-23
# \version       $Revision$
# \license       $XPC_SUITE_GPL_LICENSE$
#
#  	This file is a makefile for the libseq66 library project.  This
#  	makefile provides the skeleton needed to build the libseq66 project
#  	directory using GNU autotools.
#
#-----------------------------------------------------------------------------

#*****************************************************************************
# Packing targets.
#-----------------------------------------------------------------------------
#
#		Always use Automake in foreign mode (adding foreign to
#		AUTOMAKE_OPTIONS in Makefile.am). Otherwise, it requires too many
#		boilerplate files from the GNU coding standards that aren't useful to
#		us. 
#
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2 subdir-objects
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#*****************************************************************************
# EXTRA_DIST
#-----------------------------------------------------------------------------

EXTRA_DIST =

#*****************************************************************************
# SUBDIRS
#-----------------------------------------------------------------------------

SUBDIRS = include src

#*****************************************************************************
# DIST_SUBDIRS
#-----------------------------------------------------------------------------
#
#      DIST_SUBDIRS is used by targets that need to recurse into /all/
#      directories, even those which have been conditionally left out of the
#      build.
#
#      Precisely, DIST_SUBDIRS is used by:
#
#         -   make dist
#         -   make distclean
#         -   make maintainer-clean.
#
#      All other recursive targets use SUBDIRS.
#
#-----------------------------------------------------------------------------

DIST_SUBDIRS = $(SUBDIRS)

#*****************************************************************************
# all-local
#-----------------------------------------------------------------------------

all-local:
	@echo "Top source-directory 'top_srcdir' is $(top_srcdir)"
	@echo "* * * * * All libseq66 build items completed * * * * *"

#******************************************************************************
# Directories
#------------------------------------------------------------------------------

show:
	@echo "Install directories:"
	@echo
	@echo "prefix        = $(prefix)"
	@echo "datadir       = $(datadir)"
	@echo "datarootdir   = $(datarootdir)"
	@echo "libdir        = $(libdir)"
	@echo "localedir     = $(localedir)"
	@echo
	@echo "Local directories:"
	@echo
	@echo "top_srcdir    = $(top_srcdir) [project root directory]"
	@echo "srcdir    		= $(srcdir)"
	@echo "top_builddir  = $(top_builddir)"
	@echo "builddir      = $(builddir)"

#*****************************************************************************
# Makefile.am (libseq66)
#-----------------------------------------------------------------------------
# vim: ts=3 sw=3 noet ft=automake
#-----------------------------------------------------------------------------
