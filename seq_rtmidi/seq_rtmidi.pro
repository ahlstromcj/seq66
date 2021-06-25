#******************************************************************************
# seq_rtmidi.pro (pseq66)
#------------------------------------------------------------------------------
##
# \file       	seq_rtmidi.pro
# \library    	qpseq66 application
# \author     	Chris Ahlstrom
# \date       	2020-05-29
# \update      2020-06-01
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# Created by and for Qt Creator. This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.
#
# Important:
#
#  This project file is designed only for Qt 5 (and above?).
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

TEMPLATE = lib
CONFIG += staticlib config_prl qtc_runnable
TARGET = seq_rtmidi

# These are needed to set up seq66_platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

DEFINES += "SEQ66_MIDILIB=rtmidi"
DEFINES += "SEQ66_RTMIDI_SUPPORT=1"

HEADERS += \
 include/mastermidibus_rm.hpp \
 include/midibus_rm.hpp \
 include/midi_alsa.hpp \
 include/midi_alsa_info.hpp \
 include/midi_api.hpp \
 include/midi_info.hpp \
 include/midi_jack.hpp \
 include/midi_jack_data.hpp \
 include/midi_jack_info.hpp \
 include/midi_probe.hpp \
 include/rterror.hpp \
 include/rtmidi.hpp \
 include/rtmidi_info.hpp \
 include/seq66_rtmidi_features.h

# Mac OSX and Windows currently are not supported by the internal rtmidi
# library.  Use the portmidi build, not the rtmidi build.

SOURCES += \
 src/mastermidibus.cpp \
 src/midibus.cpp \
 src/midi_alsa.cpp \
 src/midi_alsa_info.cpp \
 src/midi_api.cpp \
 src/midi_info.cpp \
 src/midi_jack.cpp \
 src/midi_jack_info.cpp \
 src/midi_probe.cpp \
 src/rtmidi.cpp \
 src/rtmidi_info.cpp \
 src/rtmidi_types.cpp

# Note that the seq66-config.h file in ../include/qt/rtmidi, though based on a
# bootstrap of the release mode of the GNU Autotools-built version of
# seq66-config.h, is hardwired and must be updated/edited manually.  This is
# the simplest way to provide a Qt-only rtmidi build for those who don't wish
# to install autotools.

INCLUDEPATH = ../include/qt/rtmidi include ../libseq66/include

#******************************************************************************
# seq_rtmidi.pro (qpseq66)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
