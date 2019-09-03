#******************************************************************************
# seq_qt5.pro (qplseq66)
#------------------------------------------------------------------------------
##
# \file       	seq_qt5.pro
# \library    	qplseq66 application
# \author     	Chris Ahlstrom
# \date       	2018-04-08
# \update      2019-08-05
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# Created by and for Qt Creator This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.
#
# Important:
#
#  This project file is designed only for Qt 5 (and above?).
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

QT += core gui widgets
TEMPLATE = lib
CONFIG += staticlib config_prl qtc_runnable

# These are needed to set up seq66_platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
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
 forms/qseditoptions.ui \
 forms/qseqeditex.ui \
 forms/qseqeditframe.ui \
 forms/qseqeditframe64.ui \
 forms/qsliveframe.ui \
 forms/qslivegrid.ui \
 forms/qsmainwnd.ui \
 forms/qsbuildinfo.ui \
 forms/qseqeventframe.ui \
 forms/qsetmaster.ui

# RESOURCES += src/qseq66.qrc

HEADERS += \
 include/gui_palette_qt5.hpp \
 include/qclocklayout.hpp \
 include/qbase.hpp \
 include/qeditbase.hpp \
 include/qinputcheckbox.hpp \
 include/qlfoframe.hpp \
 include/qliveframeex.hpp \
 include/qmutemaster.hpp \
 include/qperfbase.hpp \
 include/qperfeditex.hpp \
 include/qperfeditframe64.hpp \
 include/qperfnames.hpp \
 include/qperfroll.hpp \
 include/qperftime.hpp \
 include/qplaylistframe.hpp \
 include/qrollframe.hpp \
 include/qsabout.hpp \
 include/qscrollmaster.h \
 include/qseditoptions.hpp \
 include/qseqbase.hpp \
 include/qseqdata.hpp \
 include/qseqeditex.hpp \
 include/qseqeditframe.hpp \
 include/qseqeditframe64.hpp \
 include/qseqframe.hpp \
 include/qseqkeys.hpp \
 include/qseqroll.hpp \
 include/qseqstyle.hpp \
 include/qseqtime.hpp \
 include/qseventslots.hpp \
 include/qskeymaps.hpp \
 include/qsliveframe.hpp \
 include/qslivegrid.hpp \
 include/qsmacros.hpp \
 include/qsmaintime.hpp \
 include/qsmainwnd.hpp \
 include/qstriggereditor.hpp \
 include/qt5_helpers.hpp \
 include/qsbuildinfo.hpp \
 include/qseqeventframe.hpp \
 include/qsetmaster.hpp

SOURCES += src/gui_palette_qt5.cpp \
 src/qbase.cpp \
 src/qeditbase.cpp \
 src/qclocklayout.cpp \
 src/qinputcheckbox.cpp \
 src/qlfoframe.cpp \
 src/qliveframeex.cpp \
 src/qmutemaster.cpp
 src/qperfbase.cpp \
 src/qperfeditex.cpp \
 src/qperfeditframe64.cpp \
 src/qperfnames.cpp \
 src/qperfroll.cpp \
 src/qperftime.cpp \
 src/qplaylistframe.cpp \
 src/qrollframe.cpp \
 src/qsabout.cpp \
 src/qscrollmaster.cpp \
 src/qseditoptions.cpp \
 src/qseqbase.cpp \
 src/qseqdata.cpp \
 src/qseqeditex.cpp \
 src/qseqeditframe.cpp \
 src/qseqeditframe64.cpp \
 src/qseqframe.cpp \
 src/qseqkeys.cpp \
 src/qseqroll.cpp \
 src/qseqstyle.cpp \
 src/qseqtime.cpp \
 src/qseventslots.cpp \
 src/qskeymaps.cpp \
 src/qsliveframe.cpp \
 src/qslivegrid.cpp \
 src/qsmaintime.cpp \
 src/qsmainwnd.cpp \
 src/qstriggereditor.cpp \
 src/qt5_helpers.cpp \
 src/qsbuildinfo.cpp \
 src/qseqeventframe.cpp \
 src/qsetmaster.cpp

# The output of the uic command goes to the seq_qt5/forms directory in
# the shadow directory, and cannot be located unless the OUT_PWD macro
# is used to find that directory.

INCLUDEPATH = \
 ../include/qt/portmidi \
 include \
 ../libseq66/include \
 ../seq_portmidi/include \
 ../resources \
 $$OUT_PWD

#******************************************************************************
# seq_qt5.pro (plseq66)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
