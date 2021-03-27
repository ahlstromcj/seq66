#******************************************************************************
# Makefile.am (seq66/data)
#------------------------------------------------------------------------------
##
# \file     	Makefile.am
# \library     seq66
# \author      Chris Ahlstrom
# \date        2018-01-02
# \updates     2020-08-15
# \version     $Revision$
# \license     $XPC_SUITE_GPL_LICENSE$
#
#   	This module provides an Automake makefile for the project's
#     seq66/data directory.  These are files we want to copy to
#     /usr/share/seq66 upon installation.
#
#------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)

#******************************************************************************
# EXTRA_DIST
#------------------------------------------------------------------------------
#
# 		Note that we have to add the appropriate wildcards to ensure these files
# 		get distributed.
#
#------------------------------------------------------------------------------

EXTRA_DIST = README readme.* license.txt linux/*.*  midi/*.* samples/*.* win/*.*

#*****************************************************************************
#	Macros
#-----------------------------------------------------------------------------
#
#	This section is recommended by:
#
#		http://olympus.het.brown.edu/cgi-bin/info2www?(gettext)Makefile
#
#-----------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------
#
# Common prefix for installation directories.  This directory must exist when
# you start the install.
#
# prefix = /usr/local
# datarootdir = $(prefix)/share
# datadir = $(datarootdir)
#
# Where to put the Info files.
#
# infodir = $(datarootdir)/info
#
#------------------------------------------------------------------------------

prefix = @prefix@
datadir = @datadir@
datarootdir = @datarootdir@
pkgdatadir = @pkgdatadir@
seq66datadir = @seq66datadir@

# datarootdir = $(prefix)/share

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#------------------------------------------------------------------------------
# install-data files
#------------------------------------------------------------------------------
#
# seq66dir = $(datarootdir)/seq66
# seq66_DATA = GM_PSS-790_Multi.ini
#
#		Instead we use variables defined in configure.ac for now.
#
#------------------------------------------------------------------------------

#******************************************************************************
# Installing documentation and other "data" files.
#------------------------------------------------------------------------------
#
#     We need to add an install-data-hook to copy the generated
#     documentation directories to the destination directory.  The normal
#     method doesn't work because /usr/bin/install will only install files,
#     and automake doesn't give it the switch needed to install directories.
#
#     Also, since we don't always build the documentation, we copied the
#     commands from doc/Makefile.am to here.
#
#------------------------------------------------------------------------------

install-data-local:
	@echo "Copying Seq66 data to $(seq66ddatadir)..."
	mkdir -p $(seq66datadir)
	cp -r -p $(top_builddir)/data/* $(seq66datadir)
	@echo "Copying Seq66 documentation to $(seq66docdir)..."
	mkdir -p $(seq66docdir)
	cp -r -p $(top_srcdir)/doc/*.pdf $(seq66docdir)
	cp -r -p $(top_srcdir)/doc/*.ods $(seq66docdir)

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------

uninstall-hook:
	rm -rf $(seq66datadir)

#******************************************************************************
# Makefile.am (seq66/include)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 noet ft=automake
#------------------------------------------------------------------------------
