#******************************************************************************
# Seqtool.pro (Seqtool)
#------------------------------------------------------------------------------
##
# \file         Seqtool.pro
# \library    	seqtool application
# \author     	Chris Ahlstrom
# \date       	2018-11-15
# \update       2019-04-11
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# Created by and for Qt Creator This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.
#
# This project file is designed only for Qt 5 (and above?).
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

QT += core gui widgets
TARGET = seqtool
TEMPLATE += app
CONFIG += static qtc_runnable console c++14

# These are needed to set up seq66_platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

# Target file directory:
# DESTDIR = bin
#
# Intermediate object files directory:
# OBJECTS_DIR = generated_files
#
# Intermediate moc files directory:
# MOC_DIR = moc
#
# Not UIC_DIR :-D

UI_DIR = forms

FORMS += forms/qtestframe.ui

# RESOURCES += nothing.qrc

HEADERS += \
 include/converter.hpp \
 include/faker.hpp \
 include/gdk_basic_keys.hpp \
 include/midi_control_helpers.hpp \
 include/midi_control_unit_test.hpp \
 include/optionsfile.hpp \
 include/qtcore_task.hpp \
 include/qtestframe.hpp \
 include/unit_tests.hpp \
 include/util_unit_test.hpp \

SOURCES += \
 src/converter.cpp \
 src/faker.cpp \
 src/gdk_basic_keys.cpp \
 src/midi_control_helpers.cpp \
 src/midi_control_unit_test.cpp \
 src/optionsfile.cpp \
 src/qtcore_task.cpp \
 src/qtestframe.cpp \
 src/seqtool.cpp \
 src/unit_tests.cpp \
 src/util_unit_test.cpp

# The output of the uic command goes to the seq_qt5/forms directory in
# the shadow directory, and cannot be located unless the OUT_PWD macro
# is used to find that directory.

INCLUDEPATH = \
 ../include/qt/portmidi \
 ../libseq66/include \
 ../seq_portmidi/include \
 include \
 $$OUT_PWD

# Sometimes some midifile and rect member functions cannot be found at link
# time, and this is worse with static linkage of our internal libraries.
# So we add the linker options --start-group and --end-group, as discussed
# in this interesting article.
#
# https://eli.thegreenplace.net/2013/07/09/library-order-in-static-linking

win32:CONFIG(release, debug|release) {
 LIBS += \
  -Wl,--start-group \
  -L$$OUT_PWD/../libseq66/release -lseq66 \
  -L$$OUT_PWD/../seq_portmidi/release -lseq_portmidi \
  -Wl,--end-group
}
else:win32:CONFIG(debug, debug|release) {
 LIBS += \
  -Wl,--start-group \
  -L$$OUT_PWD/../libseq66/debug -lseq66 \
  -L$$OUT_PWD/../seq_portmidi/debug -lseq_portmidi \
  -Wl,--end-group
}
else:unix {
LIBS += \
 -Wl,--start-group \
 -L$$OUT_PWD/../libseq66 -lseq66 \
 -L$$OUT_PWD/../seq_portmidi -lseq_portmidi \
 -Wl,--end-group
}

DEPENDPATH += \
 $$PWD/../libseq66 \
 $$PWD/../seq_portmidi

# Works in Linux with "CONFIG += debug".

unix {
 PRE_TARGETDEPS += \
  $$OUT_PWD/../libseq66/libseq66.a \ 
  $$OUT_PWD/../seq_portmidi/libseq_portmidi.a
}

# May consider adding:  /usr/include/lash-1.0 and -llash

unix:!macx: LIBS += \
 -lasound \
 -ljack \
 -lrt

windows: LIBS += -lwinmm

#******************************************************************************
# Seqtool.pro (Seqtool)
#------------------------------------------------------------------------------
# vim: ts=4 sw=4 ft=automake
#------------------------------------------------------------------------------
