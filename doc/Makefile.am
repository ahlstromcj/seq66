#*****************************************************************************
# Makefile.am (seq66)
#-----------------------------------------------------------------------------
##
# \file        Makefile.am
# \library     seq66
# \author      Chris Ahlstrom
# \date        2020-10-02
# \updates     2022-01-24
# \version     $Revision$
# \license     $XPC_SUITE_GPL_LICENSE$
#
#     This Makefile manages the "doc" directory.  It supports a number of
#     sub-projects
#
#     ca 2022-01-24 Fixes found while working on issue #45.
#
#-----------------------------------------------------------------------------

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = make_dox *.cfg dox/*.dox *.pdf *.ods

#*****************************************************************************
# SUBDIRS
#-----------------------------------------------------------------------------
#
# We now make the Doxygen documentation only manually: dox removed.
# Actually we also make the LaTeX/PDF documentation manually as well (to
# avoid checking in the binary/pdf file all the time.
#
#-----------------------------------------------------------------------------

SUBDIRS = latex

#*****************************************************************************
# DIST_SUBDIRS
#-----------------------------------------------------------------------------

DIST_SUBDIRS = $(SUBDIRS)

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
datadir = @datadir@
docdir = @docdir@
top_srcdir = @top_srcdir@
datarootdir = @datarootdir@
seq66docdir = @seq66docdir@

#******************************************************************************
# Installing documentation
#------------------------------------------------------------------------------
#
# 	   We need to add an install-data-hook to copy the generated
# 	   documentation directories to the destination directory.  The normal
# 	   method doesn't work because /usr/bin/install will only install files,
# 	   and automake doesn't give it the switch needed to install directories.
#
# 	   Also, since we don't always build the documentation, we moved these
# 	   commands to data/Makefile.am.
#
#------------------------------------------------------------------------------
#
# install-data-local:
#	@echo "Copying Seq66 documentation to $(seq66docdir)..."
#	mkdir -p $(seq66docdir)
#	cp -r -p $(top_srcdir)/doc/*.pdf $(seq66docdir)
#	cp -r -p $(top_srcdir)/doc/*.ods $(seq66docdir)
#
#------------------------------------------------------------------------------

#*******************************************************************************
# uninstall-hook
#-------------------------------------------------------------------------------
#
# uninstall-hook:
#	rm -rf $(seq66docdir)
#
#-------------------------------------------------------------------------------

#****************************************************************************
# Makefile.am (seq66 top-level)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=automake
#----------------------------------------------------------------------------
