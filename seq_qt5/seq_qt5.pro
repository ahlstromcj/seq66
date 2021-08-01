#******************************************************************************
# seq_qt5.pro (qpseq66)
#------------------------------------------------------------------------------
##
# \file           seq_qt5.pro
# \library        qpseq66 application
# \author         Chris Ahlstrom
# \date           2018-04-08
# \update         2021-05-15
# \version        $Revision$
# \license        $XPC_SUITE_GPL_LICENSE$
#
# Created by and for Qt Creator This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.
#
# Important:
#
#  This project file is designed only for Qt 5 (and above?).
#
# Removed:
#
#     qseqeditframe.cpp/.hpp/.ui
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

QT += core gui widgets
TEMPLATE = lib
CONFIG += staticlib config_prl qtc_runnable c++14

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

UI_DIR = forms

FORMS += forms/qlfoframe.ui \
 forms/qliveframeex.ui \
 forms/qmutemaster.ui \
 forms/qperfeditex.ui \
 forms/qperfeditframe64.ui \
 forms/qplaylistframe.ui \
 forms/qsabout.ui \
 forms/qsbuildinfo.ui \
 forms/qseditoptions.ui \
 forms/qseqeditex.ui \
 forms/qseqeditframe64.ui \
 forms/qseqeventframe.ui \
 forms/qsessionframe.ui \
 forms/qsetmaster.ui \
 forms/qslivegrid.ui \
 forms/qsmainwnd.ui

HEADERS += include/gui_palette_qt5.hpp \
 include/palettefile.hpp \
 include/qbase.hpp \
 include/qclocklayout.hpp \
 include/qeditbase.hpp \
 include/qinputcheckbox.hpp \
 include/qlfoframe.hpp \
 include/qliveframeex.hpp \
 include/qloopbutton.hpp \
 include/qmutemaster.hpp \
 include/qperfbase.hpp \
 include/qperfeditex.hpp \
 include/qperfeditframe64.hpp \
 include/qperfnames.hpp \
 include/qperfroll.hpp \
 include/qperftime.hpp \
 include/qplaylistframe.hpp \
 include/qsabout.hpp \
 include/qsbuildinfo.hpp \
 include/qscrollmaster.h \
 include/qseditoptions.hpp \
 include/qseqbase.hpp \
 include/qseqdata.hpp \
 include/qseqeditex.hpp \
 include/qseqeditframe64.hpp \
 include/qseqeventframe.hpp \
 include/qseqframe.hpp \
 include/qseqkeys.hpp \
 include/qseqroll.hpp \
 include/qseqstyle.hpp \
 include/qseqtime.hpp \
 include/qsessionframe.hpp \
 include/qsetmaster.hpp \
 include/qseventslots.hpp \
 include/qskeymaps.hpp \
 include/qslivebase.hpp \
 include/qslivegrid.hpp \
 include/qslotbutton.hpp \
 include/qsmaintime.hpp \
 include/qsmainwnd.hpp \
 include/qstriggereditor.hpp \
 include/qt5_helpers.hpp \
 include/qt5nsmanager.hpp

SOURCES += src/gui_palette_qt5.cpp \
 src/palettefile.cpp \
 src/qbase.cpp \
 src/qclocklayout.cpp \
 src/qeditbase.cpp \
 src/qinputcheckbox.cpp \
 src/qlfoframe.cpp \
 src/qliveframeex.cpp \
 src/qloopbutton.cpp \
 src/qmutemaster.cpp \
 src/qperfbase.cpp \
 src/qperfeditex.cpp \
 src/qperfeditframe64.cpp \
 src/qperfnames.cpp \
 src/qperfroll.cpp \
 src/qperftime.cpp \
 src/qplaylistframe.cpp \
 src/qsabout.cpp \
 src/qsbuildinfo.cpp \
 src/qscrollmaster.cpp \
 src/qseditoptions.cpp \
 src/qseqbase.cpp \
 src/qseqdata.cpp \
 src/qseqeditex.cpp \
 src/qseqeditframe64.cpp \
 src/qseqeventframe.cpp \
 src/qseqframe.cpp \
 src/qseqkeys.cpp \
 src/qseqroll.cpp \
 src/qseqstyle.cpp \
 src/qseqtime.cpp \
 src/qsessionframe.cpp \
 src/qsetmaster.cpp \
 src/qseventslots.cpp \
 src/qskeymaps.cpp \
 src/qslivebase.cpp \
 src/qslivegrid.cpp \
 src/qslotbutton.cpp \
 src/qsmaintime.cpp \
 src/qsmainwnd.cpp \
 src/qstriggereditor.cpp \
 src/qt5_helpers.cpp \
 src/qt5nsmanager.cpp

# The output of the uic command goes to the seq_qt5/forms directory in
# the shadow directory, and cannot be located unless the OUT_PWD macro
# is used to find that directory.

INCLUDEPATH = include \
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
