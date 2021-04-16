#*******************************************************************************
# Makefile.am
#-------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	doc/dox
# \author     	Chris Ahlstrom
# \date       	2015-09-10
# \update     	2020-11-25
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		Makefile for the doc/dox subdirectory of this package.  We had to do a
# 		little two-stepping to make sure that the documentation is built
# 		everytime we touch one of the doxygen files, and NOT when we touch one of
# 		the source-code files.  Tired of checking in the PDF file all the time!
#
# 		Also, we leave it to the user to decide if the programmer's documentation
# 		should be built.  The PDFs are not checked into git.
#
#-------------------------------------------------------------------------------

#*****************************************************************************
# Packing/cleaning targets
#-----------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2
MAINTAINERCLEANFILES = Makefile.in dox-warnings.log
CLEANFILES = dox-stamp \
 seq_rtmidi/seq66_lib_seq_rtmidi.pdf \
 libsessions/seq66_lib_libsessions.pdf \
 seq_portmidi/seq66_lib_seq_portmidi.pdf \
 libseq66/seq66_lib_libseq66.pdf

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = make_dox make-helper optimize *.cfg *.dox # *.pdf

#******************************************************************************
# Local project directories
#------------------------------------------------------------------------------

top_srcdir = @top_srcdir@
srcdir = @srcdir@

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
datadir = @datadir@
docdir = @docdir@
datarootdir = @datarootdir@
seq66docdir = @seq66docdir@

#------------------------------------------------------------------------------
# The $(srcdir) component points us to the correct place whether the build is
# in-source or out-of-source.  Can't use pushd/popd because they are bash
# builtins and are not available, except inside a script.
#------------------------------------------------------------------------------

all-local:
	$(srcdir)/make_dox $(srcdir) libseq66
	$(srcdir)/make_dox $(srcdir) libsessions
	$(srcdir)/make_dox $(srcdir) seq_portmidi
	$(srcdir)/make_dox $(srcdir) seq_rtmidi

clean-local:
	$(srcdir)/make_dox $(srcdir) libseq66 clean
	$(srcdir)/make_dox $(srcdir) libsessions clean
	$(srcdir)/make_dox $(srcdir) seq_portmidi clean
	$(srcdir)/make_dox $(srcdir) seq_rtmidi clean

#******************************************************************************
# Installing documentation
#------------------------------------------------------------------------------
#
# 	   We need to add an install-data-hook to copy the generated
# 	   documentation directories to the destination directory.  The normal
# 	   method doesn't work because /usr/bin/install will only install files,
# 	   and automake doesn't give it the switch needed to install directories.
#
# Don't install the html files:
#
# 		cp -r -p $(top_builddir)/doc/dox/html $(xpcdoxygendir)
#
#------------------------------------------------------------------------------

install-data-local:
	@echo "Copying Seq66 documentation to $(DESTDIR)$(seq66docdir)..."
	mkdir -p $(DESTDIR)$(seq66docdir)
	cp -r -p $(top_builddir)/doc/*.pdf $(DESTDIR)$(seq66docdir)
	cp -r -p $(top_srcdir)/doc/*.ods $(DESTDIR)$(seq66docdir)

#*******************************************************************************
# uninstall-hook
#-------------------------------------------------------------------------------
#
#		Quick and dirty method used.
#
#-------------------------------------------------------------------------------

uninstall-hook:
	rm -rf $(DESTDIR)$(seq66docdir)

#******************************************************************************
# Makefile.am (dox)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 noet ft=automake
#------------------------------------------------------------------------------
