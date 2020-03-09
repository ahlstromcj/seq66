#******************************************************************************
# libsessions.pro (qpseq66)
#------------------------------------------------------------------------------
##
# \file        libsessions.pro
# \library     qseq66 and qpseq66 application
# \author      Chris Ahlstrom
# \date        2020-03-08
# \update      2020-03-08
# \version     $Revision$
# \license     $XPC_SUITE_GPL_LICENSE$
#
# Important:
#
#      This project file is designed only for Qt 5 (and above?).
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

TEMPLATE = lib
CONFIG += staticlib config_prl qtc_runnable c++14
TARGET = sessions

# These are needed to set up seq66_platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

HEADERS += include/nsm/nsm.hpp \
 include/nsm/nsmclient.hpp \
 include/nsm/nsmmessages.hpp

SOURCES += src/nsm/nsm.cpp \
 src/nsm/nsmclient.cpp \
 src/nsm/nsmmessages.cpp

# INCLUDEPATH = ../include/qt/portmidi ../seq_portmidi/include include

#******************************************************************************
# libsessions.pro (qpseq66)
#------------------------------------------------------------------------------
# vim: ts=4 sw=4 ft=automake
#------------------------------------------------------------------------------
