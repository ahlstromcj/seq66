#*****************************************************************************
# Makefile.am (seq66)
#-----------------------------------------------------------------------------
##
# \file        Makefile.am
# \library     seq66
# \author      Chris Ahlstrom
# \date        2018-11-11
# \updates     2020-02-09
# \version     $Revision$
# \license     $XPC_SUITE_GPL_LICENSE$
#
#     This is the top-level project makefile for the seq66 project.
#     This makefile provides the skeleton needed to build the library
#     and application directories using GNU autotools.  It supports a number
#     of sub-projects
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Packing targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in Makefile $(AUX_DIST)
MOSTLYCLEANFILES = *~

#*****************************************************************************
# Extra files in the top-level directory
#-----------------------------------------------------------------------------

EXTRA_DIST = bootstrap pack README VERSION INSTALL NEWS ChangeLog

#*****************************************************************************
# Packaging
#-----------------------------------------------------------------------------
#
#     This section is recommended for the top-level Makefile.am by the
#     reference.
#
#-----------------------------------------------------------------------------

PACKAGE = @PACKAGE@
VERSION = @VERSION@

SEQ66_API_MAJOR = @SEQ66_API_MAJOR@
SEQ66_API_MINOR = @SEQ66_API_MINOR@
SEQ66_API_PATCH = @SEQ66_API_PATCH@
SEQ66_API_VERSION = @SEQ66_API_VERSION@

SEQ66_LT_CURRENT = @SEQ66_LT_CURRENT@
SEQ66_LT_REVISION = @SEQ66_LT_REVISION@
SEQ66_LT_AGE = @SEQ66_LT_AGE@

#*****************************************************************************
# Installed directories
#-----------------------------------------------------------------------------
#
# The 'libdir' define is necessary to cause the proper subdirectory to
# be made during installation.  'seq66libdir' is defined in the
# configure.ac script.  The 'libdir' define is needed for work with
# libtool.  Not sure about 'pkglibdr'.
#
#     pkglibdir=$(seq66libdir)
#
#	Directories and macros:
#
#		prefix          = /usr/local
#		libdir          = /usr/local/lib/seq66
#		datadir         = /usr/local/share
#		datarootdir     = /usr/local/share
#		seq66libdir     = /usr/local/lib/seq66
#		seq66docdir     = /usr/local/share/doc/seq66.1
#		seq66doxygendir = doc
#		seq66includedir = /usr/local/include/seq66.1
#		localedir       = /usr/local/share/locale
#
#  'localedir' is the normal system directory for installed localization
#  files.
#
#-----------------------------------------------------------------------------

prefix = @prefix@
libdir = @seq66libdir@
datadir = @datadir@
datarootdir = @datarootdir@
seq66docdir = @seq66docdir@
seq66doxygendir = @seq66doxygendir@
seq66includedir = @seq66includedir@
seq66libdir = @seq66libdir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------
#
#		top_srcdir    = ../../.. [XPC Basic directory]
#		builddir      = /home/ahlstrom/ca/mls/git/XPC Basic-1.1/debug
#
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

#******************************************************************************
# aclocal support
#------------------------------------------------------------------------------

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

#*****************************************************************************
# libtool
#-----------------------------------------------------------------------------
#	${SHELL} $(top_srcdir)/config.status --recheck
#-----------------------------------------------------------------------------

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libtool: $(LIBTOOL_DEPS)
	$(top_srcdir)/config.status --recheck

#*****************************************************************************
# SUBDIRS
#-----------------------------------------------------------------------------
#
#  We decided to build the Doxygen documentation only manually, and have
#  commented it out.
#
#  # doc/dox
#
#  Note the order of these tests is important to handling the mixing and
#  matching of MIDI engine versus user-interface.
#
#     RTMIDI/GTKMM
#     RTMIDI/QT5
#     PORTMIDI/GTKMM
#     Legacy ALSA/GTKMM
#     RTMIDI/CLI
#     PORTMIDI/WINDOWS/CLI build: QT5 Windows build is done via qmake.
#
#  See INSTALL.
#
#-----------------------------------------------------------------------------

# SUBDIRS = resources/pixmaps libseq66

SUBDIRS = m4 libseq66 resources/pixmaps

if BUILD_RTMIDI
SUBDIRS += seq_rtmidi
endif

if BUILD_PORTMIDI
SUBDIRS += seq_portmidi
endif

if BUILD_QTMIDI
SUBDIRS += seq_qt5 Seq66qt5
endif

if BUILD_RTCLI
SUBDIRS += Seq66cli
endif

if BUILD_SEQTOOL
SUBDIRS += Seqtool
endif

SUBDIRS += man data

#*****************************************************************************
# DIST_SUBDIRS
#-----------------------------------------------------------------------------

DIST_SUBDIRS = $(SUBDIRS)

#*****************************************************************************
# all-local
#-----------------------------------------------------------------------------

all-local:
	@echo "Top source-directory 'top_srcdir' is $(top_srcdir)"
	@echo "* * * * * All build items completed * * * * *"

#******************************************************************************
# Debugging targets
#------------------------------------------------------------------------------

show:
	@echo "Install directories:"
	@echo
	@echo "prefix                    = $(prefix)"
	@echo "datadir                   = $(datadir)"
	@echo "datarootdir               = $(datarootdir)"
	@echo "libdir                    = $(libdir)"
	@echo "seq66libdir               = $(seq66libdir)"
	@echo "seq66datadir              = $(seq66datadir)"
	@echo "seq66docdir               = $(seq66docdir)"
	@echo "seq66doxygendir           = $(seq66doxygendir)"
	@echo "seq66includedir           = $(seq66includedir)"
	@echo "localedir                 = $(localedir)"
	@echo
	@echo "Local directories:"
	@echo
	@echo "top_srcdir                = $(top_srcdir) [project root directory]"
	@echo "srcdir                    = $(srcdir)"
	@echo "top_builddir              = $(top_builddir)"
	@echo "builddir                  = $(builddir)"

showver:
	@echo "PACKAGE                   = $(PACKAGE)"
	@echo "VERSION                   = $(VERSION)"
	@echo "SEQ66_API_MAJOR           = $(SEQ66_API_MAJOR)"
	@echo "SEQ66_API_MINOR           = $(SEQ66_API_MINOR)"
	@echo "SEQ66_API_PATCH           = $(SEQ66_API_PATCH)"
	@echo "SEQ66_API_VERSION         = $(SEQ66_API_VERSION)"
	@echo "SEQ66_LT_CURRENT          = $(SEQ66_LT_CURRENT)"
	@echo "SEQ66_LT_REVISION         = $(SEQ66_LT_REVISION)"
	@echo "SEQ66_LT_AGE              = $(SEQ66_LT_AGE)"

#****************************************************************************
# Makefile.am (seq66 top-level)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=automake
#----------------------------------------------------------------------------
