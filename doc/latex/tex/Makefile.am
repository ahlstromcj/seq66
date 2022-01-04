#*******************************************************************************
# Makefile.am
#-------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	Documents
# \author     	Chris Ahlstrom
# \date       	2020-10-02
# \update     	2022-01-03
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		Makefile for LaTeX-based documentation.  It now uses latexmk (a
# 		well-known Perl script) instead of using pdflatex and makeindex
# 		directly.
#
#-------------------------------------------------------------------------------

#******************************************************************************
#  EXTRA_DIST
#------------------------------------------------------------------------------

EXTRA_DIST = *.tex

#******************************************************************************
# Install directories
#------------------------------------------------------------------------------

prefix = @prefix@
datadir = @datadir@
docdir = @docdir@
datarootdir = @datarootdir@
seq66docdir = @seq66docdir@
top_srcdir = @top_srcdir@
builddir = @abs_top_builddir@

.PHONY: clean

all: manual

manual: tex-stamp
	latexmk -g -silent -pdf seq66-user-manual
	cp seq66-user-manual.pdf $(top_srcdir)/data/share/doc

tex-stamp:
	touch tex-stamp

# Or -C instead?

clean: clean-dvi clean-pdf clean-ps
	latexmk -c
	rm -f *.4tc
	rm -f *.xref
	rm -f tex-stamp

clean-dvi:
	-rm -f *.dvi

clean-pdf:
	-rm -f *.pdf

clean-ps:
	-rm -f *.ps

#******************************************************************************
# vim: ts=3 sw=3 noet ft=automake
#------------------------------------------------------------------------------
