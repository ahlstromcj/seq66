#******************************************************************************
# Makefile.am (seq66/data)
#------------------------------------------------------------------------------
##
# \file     	Makefile.am
# \library     seq66
# \author      Chris Ahlstrom
# \date        2018-01-02
# \updates     2022-01-04
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

EXTRA_DIST = readme.* license.* linux/*.* midi/*.* samples/*.* win/*.* share/*

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

datafolder = $(DESTDIR)$(seq66datadir)
docfolder = $(DESTDIR)$(seq66docdir)

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
#		Instead we use variables defined in configure.ac for now. Also, the
#		desktop and icon handling below does not work.
#
#------------------------------------------------------------------------------
#
# desktopdir = $(datadir)/applications
# desktop_in_files = share/applications/seq66.desktop.in
# desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
# 
# icondir = $(datadir)/icons/hicolor/128x128/apps
# icon_DATA = $(top_srcdir)/data/share/icons/hicolor/128x128/apps/qseq66.png
# icondir = $(datadir)/icons/hicolor/
# icon_DATA = $(top_srcdir)/data/share/icons/hicolor/
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
	@echo "Copying Seq66 data to $(datafolder) ..."
	mkdir -p $(datafolder)
	mkdir -p $(datafolder)/linux
	mkdir -p $(datafolder)/midi
	mkdir -p $(datafolder)/pixmaps
	mkdir -p $(datafolder)/samples
	mkdir -p $(datafolder)/seq66cli
	mkdir -p $(datafolder)/win
	mkdir -p $(datafolder)/wrk
	cp -r -p $(top_builddir)/data/readme.* $(datafolder)/
	cp -r -p $(top_builddir)/data/license.* $(datafolder)/
	cp -r -p $(top_builddir)/data/linux/* $(datafolder)/linux/
	cp -r -p $(top_builddir)/data/midi/* $(datafolder)/midi/
	cp -r -p $(top_builddir)/data/pixmaps/* $(datafolder)/pixmaps/
	cp -r -p $(top_builddir)/data/samples/* $(datafolder)/samples/
	cp -r -p $(top_builddir)/data/seq66cli/* $(datafolder)/seq66cli/
	cp -r -p $(top_builddir)/data/win/* $(datafolder)/win/
	cp -r -p $(top_builddir)/data/wrk/* $(datafolder)/wrk/
	cp -p $(top_srcdir)/data/share/applications/seq66.desktop $(datadir)/applications/
	cp -r -p $(top_srcdir)/data/share/icons/* $(datadir)/icons/
	@echo "Copying Seq66 documentation to $(docfolder)..."
	mkdir -p $(docfolder)
	cp -r -p $(top_srcdir)/data/share/doc/*.pdf $(docfolder)
	cp -r -p $(top_srcdir)/data/share/doc/*.ods $(docfolder)

#******************************************************************************
# uninstall-hook
#------------------------------------------------------------------------------
#
# 	Also need to uninstall icons, desktop file, include files.
#
#------------------------------------------------------------------------------

uninstall-hook:
	rm -rf $(datafolder)
	rm -rf $(docfolder)
	rm -f $(datadir)/applications/seq66.desktop
	find $(datadir)/icons/ -iname "*seq66*.png" -exec rm -f '{}' \;

#******************************************************************************
# Makefile.am (seq66/include)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 noet ft=automake
#------------------------------------------------------------------------------
