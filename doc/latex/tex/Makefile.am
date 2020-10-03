#*******************************************************************************
# Makefile.am
#-------------------------------------------------------------------------------
##
# \file       	Makefile.am
# \library    	Documents
# \author     	Chris Ahlstrom
# \date       	2020-10-02
# \update     	2020-10-02
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# 		Makefile for LaTeX-based documentation.  It now uses latexmk (a
# 		well-known Perl script) instead of using pdflatex and makeindex
# 		directly.
#
#-------------------------------------------------------------------------------

.PHONY: clean
.PHONY: docs

all: cookbook

cookbook: tex-stamp
	latexmk -g -silent -pdf seq66-user-manual
	mv seq66-user-manual.pdf ../pdf

tex-stamp:
	touch tex-stamp

# Or -C instead?

clean: clean-dvi clean-pdf clean-ps
	latexmk -c
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
