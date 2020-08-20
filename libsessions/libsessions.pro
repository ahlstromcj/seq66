#******************************************************************************
# libsessions.pro (qpseq66)
#------------------------------------------------------------------------------
##
# \file        libsessions.pro
# \library     qseq66 and qrseq66 application
# \author      Chris Ahlstrom
# \date        2020-03-24
# \update      2020-08-20
# \version     $Revision$
# \license     $XPC_SUITE_GPL_LICENSE$
#
# Important:
#
#      This project file is designed only for Qt 5 (and above?).  However,
#      on a 32-bit Linux system with an old version of Qt (5.3), the build
#      fails due to std::unique_ptr not being defined, even with the c++11
#      flag added.
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

TEMPLATE = lib
CONFIG += staticlib config_prl qtc_runnable c++14
TARGET = seq66

# These are needed to set up seq66_platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

contains (CONFIG, rtmidi) {
   MIDILIB = rtmidi
   DEFINES += "SEQ66_MIDILIB=\\\"rtmidi\\\""
   DEFINES += "SEQ66_RTMIDI_SUPPORT=1"
} else {
   MIDILIB = portmidi
   DEFINES += "SEQ66_MIDILIB=\\\"portmidi\\\""
   DEFINES += "SEQ66_PORTMIDI_SUPPORT=1"
}

HEADERS += include/lash/lash.hpp \
 include/nsm/nsmbase.hpp \
 include/nsm/nsmclient.hpp \
 include/nsm/nsmmessages.hpp

SOURCES += src/lash/lash.cpp \
 src/nsm/nsmbase.cpp \
 src/nsm/nsmclient.cpp \
 src/nsm/nsmmessages.cpp \

INCLUDEPATH = ../include/qt/rtmidi \
 ../libseq66/include \
 ../seq_rtmidi/include \
 include

#******************************************************************************
# libsessions.pro (qpseq66)
#------------------------------------------------------------------------------
# vim: ts=4 sw=4 ft=automake
#------------------------------------------------------------------------------
