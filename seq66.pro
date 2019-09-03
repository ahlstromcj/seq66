#******************************************************************************
# qpseq66.pro (qpseq66)
#------------------------------------------------------------------------------
##
# \file         qpseq66.pro
# \library      qpseq66 application
# \author       Chris Ahlstrom
# \date         2018-11-15
# \update       2019-04-27
# \version      $Revision$
# \license      $XPC_SUITE_GPL_LICENSE$
#
#   Created for Qt Creator. This file was created for editing the project
#   sources only.  You may attempt to use it for building too, by modifying
#   this file here.
#
#   This project file is designed only for Qt 5 and, it is to be hoped, above.
#
#------------------------------------------------------------------------------

TEMPLATE = subdirs
SUBDIRS =  libseq66 seq_portmidi Seqtool seq_qt5 Seq66qt5
CONFIG += static link_prl ordered qtc_runnable c++11

# None of these seem to work on 32-bit Linux using Qt 5.3:
#
# CONFIG += c++14
# QMAKE_CXXFLAGS += -std=gnu++14

QMAKE_CXXFLAGS += -std=c++14

Seqtool.depends = libseq66
Seq66qt5.depends = libseq66 seq_portmidi seq_qt5

#******************************************************************************
# qpseq66.pro (qpseq66)
#------------------------------------------------------------------------------
# vim: ts=4 sw=4 ft=automake
#------------------------------------------------------------------------------
