#******************************************************************************
# seq_qt5.pro (qpseq66)
#------------------------------------------------------------------------------
##
# \file           seq_qt5.pro
# \library        qpseq66 application
# \author         Chris Ahlstrom
# \date           2018-04-08
# \update         2026-05-16
# \version        $Revision$
# \license        $XPC_SUITE_GPL_LICENSE$
#
# Created by and for Qt Creator This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.
#
# Important:
#
#  This project file is designed only for Qt 5 (and above).
#
# Removed:
#
#     qseqeditframe.cpp/.hpp/.ui
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

QT += core gui widgets
TEMPLATE = lib
CONFIG += staticlib config_prl qtc_runnable c++17
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050F00

# These are needed to set up seq66_platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
}

contains (CONFIG, rtmidi) {
   MIDILIB = rtmidi
   DEFINES += "SEQ66_MIDILIB=rtmidi"
   DEFINES += "SEQ66_RTMIDI_SUPPORT=1"
} else {
   MIDILIB = portmidi
   DEFINES += "SEQ66_MIDILIB=portmidi"
   DEFINES += "SEQ66_PORTMIDI_SUPPORT=1"
}

TARGET = seq_qt5

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

UI_DIR = src/gui

FORMS += src/gui/qlfoframe.ui \
 src/gui/qliveframeex.ui \
 src/gui/qmutemaster.ui \
 src/gui/qpatternfix.ui \
 src/gui/qperfeditex.ui \
 src/gui/qperfeditframe64.ui \
 src/gui/qplaylistframe.ui \
 src/gui/qsabout.ui \
 src/gui/qsappinfo.ui \
 src/gui/qsbuildinfo.ui \
 src/gui/qseditoptions.ui \
 src/gui/qseqeditex.ui \
 src/gui/qseqeditframe64.ui \
 src/gui/qseqeventframe.ui \
 src/gui/qsessionframe.ui \
 src/gui/qsetmaster.ui \
 src/gui/qslivegrid.ui \
 src/gui/qslogview.ui \
 src/gui/qsmainwnd.ui

HEADERS += src/gui/qlfoframe.hpp \
 src/gui/qliveframeex.hpp \
 src/gui/qmutemaster.hpp \
 src/gui/qpatternfix.hpp \
 src/gui/qperfeditex.hpp \
 src/gui/qperfeditframe64.hpp \
 src/gui/qplaylistframe.hpp \
 src/gui/qsabout.hpp \
 src/gui/qsappinfo.hpp \
 src/gui/qsbuildinfo.hpp \
 src/gui/qseditoptions.hpp \
 src/gui/qseqeditex.hpp \
 src/gui/qseqeditframe64.hpp \
 src/gui/qseqeventframe.hpp \
 src/gui/qsessionframe.hpp \
 src/gui/qsetmaster.hpp \
 src/gui/qslivebase.hpp \
 src/gui/qslivegrid.hpp \
 src/gui/qslogview.hpp \
 src/gui/qsmainwnd.hpp \
 include/gui_palette_qt5.hpp \
 include/palettefile.hpp \
 include/qbase.hpp \
 include/qclocklayout.hpp \
 include/qeditbase.hpp \
 include/qinputcheckbox.hpp \
 include/qloopbutton.hpp \
 include/qperfbase.hpp \
 include/qperfnames.hpp \
 include/qperfroll.hpp \
 include/qperftime.hpp \
 include/qportwidget.hpp \
 include/qscrollmaster.h \
 include/qscrollslave.h \
 include/qseqbase.hpp \
 include/qseqdata.hpp \
 include/qseqframe.hpp \
 include/qseqkeys.hpp \
 include/qseqroll.hpp \
 include/qseqtime.hpp \
 include/qseventslots.hpp \
 include/qslotbutton.hpp \
 include/qsmaintime.hpp \
 include/qstriggereditor.hpp \
 include/qt5_helper.h \
 include/qt5_helpers.hpp \
 include/qt5nsmanager.hpp

SOURCES += src/gui/qlfoframe.cpp \
 src/gui/qliveframeex.cpp \
 src/gui_palette_qt5.cpp \
 src/gui/qmutemaster.cpp \
 src/gui/qpatternfix.cpp \
 src/gui/qperfeditex.cpp \
 src/gui/qperfeditframe64.cpp \
 src/gui/qplaylistframe.cpp \
 src/gui/qsabout.cpp \
 src/gui/qsappinfo.cpp \
 src/gui/qsbuildinfo.cpp \
 src/gui/qseditoptions.cpp \
 src/gui/qseqeditex.cpp \
 src/gui/qseqeditframe64.cpp \
 src/gui/qseqeventframe.cpp \
 src/gui/qsessionframe.cpp \
 src/gui/qsetmaster.cpp \
 src/gui/qslivegrid.cpp \
 src/gui/qslogview.cpp \
 src/gui/qsmainwnd.cpp \
 src/palettefile.cpp \
 src/qbase.cpp \
 src/qclocklayout.cpp \
 src/qeditbase.cpp \
 src/qinputcheckbox.cpp \
 src/qloopbutton.cpp \
 src/qperfbase.cpp \
 src/qperfnames.cpp \
 src/qperfroll.cpp \
 src/qperftime.cpp \
 src/qportwidget.cpp \
 src/qscrollmaster.cpp \
 src/qscrollslave.cpp \
 src/qseqbase.cpp \
 src/qseqdata.cpp \
 src/qseqframe.cpp \
 src/qseqkeys.cpp \
 src/qseqroll.cpp \
 src/qseqtime.cpp \
 src/qseventslots.cpp \
 src/qslivebase.cpp \
 src/qslotbutton.cpp \
 src/qsmaintime.cpp \
 src/qstriggereditor.cpp \
 src/qt5_helpers.cpp \
 src/qt5nsmanager.cpp

#------------------------------------------------------------------------------
# The output of the uic command goes to the seq_qt5/forms directory in
# the shadow directory, and cannot be located unless the OUT_PWD macro
# is used to find that directory. Tricky stuff.
#------------------------------------------------------------------------------

INCLUDEPATH = include \
 src \
 gui \
 ../include/qt/$${MIDILIB} \
 ../libseq66/include \
 ../libsessions/include \
 ../seq_$${MIDILIB}/include \
 ../resources \
 $$OUT_PWD

#******************************************************************************
# seq_qt5.pro (qpseq66)
#------------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
