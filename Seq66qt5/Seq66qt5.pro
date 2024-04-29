#******************************************************************************
# Seq66qt5.pro (Seq66qt5)
#------------------------------------------------------------------------------
##
# \file       	Seq66qt5.pro
# \library    	seq66qt5 application
# \author     	Chris Ahlstrom
# \date       	2018-04-08
# \update      2024-04-29
# \version    	$Revision$
# \license    	$XPC_SUITE_GPL_LICENSE$
#
# Created by and for Qt Creator This file was created for editing the project
# sources only.  You may attempt to use it for building too, by modifying this
# file here.
#
# Important: This project file is designed only for Qt 5 (and above?).
#
# target.path = /usr/local/bin/
# INSTALLS += target
#
#------------------------------------------------------------------------------

message($$_PRO_FILE_PWD_)

QT += widgets gui core
TEMPLATE += app
CONFIG += static qtc_runnable c++14

# These are needed to set up seq66_platform_macros:

CONFIG(debug, debug|release) {
   DEFINES += DEBUG
} else {
   DEFINES += NDEBUG
   DEFINES += QT_NO_DEBUG
}

contains (CONFIG, rtmidi) {
   TARGET = qseq66
   MIDILIB = rtmidi
   DEFINES += "SEQ66_MIDILIB=rtmidi"
   DEFINES += "SEQ66_RTMIDI_SUPPORT=1"
} else {
   TARGET = qpseq66
   MIDILIB = portmidi
   DEFINES += "SEQ66_MIDILIB=portmidi"
   DEFINES += "SEQ66_PORTMIDI_SUPPORT=1"
   DEFINES += "MINGW_HAS_SECURE_API=1"
}

windows {
   DEFINES += "UNICODE"
   DEFINES += "_UNICODE"
}

win32 {
   DEFINES += "WIN32"
   DEFINES += "QT_NEEDS_QMAIN"
}

message($${DEFINES})

SOURCES += seq66qt5.cpp

INCLUDEPATH = \
 ../include/qt/$${MIDILIB} \
 ../libseq66/include \
 ../libsessions/include \
 ../seq_$${MIDILIB}/include \
 ../seq_qt5/include

# Sometimes some midifile and rect member functions cannot be found at link
# time, and this is worse with static linkage of our internal libraries.
# So we add the linker options --start-group and --end-group, as discussed
# in this interesting article.
#
# https://eli.thegreenplace.net/2013/07/09/library-order-in-static-linking
#

win32:CONFIG(release, debug|release) {
 LIBS += \
  -Wl,--start-group \
  -L$$OUT_PWD/../libseq66/release -lseq66 \
  -L$$OUT_PWD/../seq_$${MIDILIB}/release -lseq_$${MIDILIB} \
  -L$$OUT_PWD/../seq_qt5/release -lseq_qt5 \
  -lmingw32 -lshell32 \
  -Wl,--end-group
}
else:windows:CONFIG(debug, debug|release) {
 LIBS += \
  -Wl,--start-group \
  -L$$OUT_PWD/../libseq66/debug -lseq66 \
  -L$$OUT_PWD/../seq_$${MIDILIB}/debug -lseq_$${MIDILIB} \
  -L$$OUT_PWD/../seq_qt5/debug -lseq_qt5 \
  -Wl,--end-group
}
else:unix {
contains (CONFIG, rtmidi) {
LIBS += \
 -Wl,--start-group \
 -L$$OUT_PWD/../libsessions -lsessions \
 -L$$OUT_PWD/../libseq66 -lseq66 \
 -L$$OUT_PWD/../seq_$${MIDILIB} -lseq_$${MIDILIB} \
 -L$$OUT_PWD/../seq_qt5 -lseq_qt5 \
 -Wl,--end-group
} else {
LIBS += \
 -Wl,--start-group \
 -L$$OUT_PWD/../libseq66 -lseq66 \
 -L$$OUT_PWD/../seq_$${MIDILIB} -lseq_$${MIDILIB} \
 -L$$OUT_PWD/../seq_qt5 -lseq_qt5 \
 -Wl,--end-group
}
}

contains (CONFIG, rtmidi) {
DEPENDPATH += $$PWD/../libsessions
}

DEPENDPATH += \
 $$PWD/../libseq66 \
 $$PWD/../seq_$${MIDILIB} \
 $$PWD/../seq_qt5

# Works in Linux with "CONFIG += debug".

contains (CONFIG, rtmidi) {
PRE_TARGETDEPS += $$OUT_PWD/../libsessions/libsessions.a \ 
}

unix {

 isEmpty(PREFIX) { PREFIX = /usr/local }
 isEmpty(BINDIR) { BINDIR = $${PREFIX}/bin }
 isEmpty(DATADIR) { DATADIR = $${PREFIX}/share }

 #DEFINES += DATADIR=\"$${DATADIR}\"

 # make install

 INSTALLS += target

# desktop icon appdata icon_scalable mimeinfo mimetypes mimetypes_scalable

 target.path = $${BINDIR}

## TO DO.
##
 desktop.path = $${DATADIR}/applications
 desktop.files += ../data/share/applications/seq66.desktop
#   icon.path = $${DATADIR}/icons/hicolor/32x32/apps
#   icon.files += images/$${NAME}.png
#   icon_scalable.path = $${DATADIR}/icons/hicolor/scalable/apps
#   icon_scalable.files += images/$${NAME}.svg
#   appdata.path = $${DATADIR}/metainfo
#   appdata.files += appdata/$${NAME}.appdata.xml
#   mimeinfo.path = $${DATADIR}/mime/packages
#   mimeinfo.files += mimetypes/$${NAME}.xml
#   mimetypes.path = $${DATADIR}/icons/hicolor/32x32/mimetypes
#   mimetypes.files += \
#       mimetypes/application-x-$${NAME}-session.png \
#       mimetypes/application-x-$${NAME}-template.png \
#       mimetypes/application-x-$${NAME}-archive.png
#   mimetypes_scalable.path = $${DATADIR}/icons/hicolor/scalable/mimetypes
#   mimetypes_scalable.files += \
#       mimetypes/application-x-$${NAME}-session.svg \
#       mimetypes/application-x-$${NAME}-template.svg \
#       mimetypes/application-x-$${NAME}-archive.svg

 PRE_TARGETDEPS += \
  $$OUT_PWD/../libseq66/libseq66.a \ 
  $$OUT_PWD/../seq_$${MIDILIB}/libseq_$${MIDILIB}.a \ 
  $$OUT_PWD/../seq_qt5/libseq_qt5.a
}

# Note the inclusion of liblo (OSC library).
# We may consider adding:  /usr/include/lash-1.0 and -llash

contains (CONFIG, rtmidi) {
LIBS += -lasound -ljack -llo -lrt
} else {
unix:!macx: LIBS += -lasound -ljack -lrt
}

windows: LIBS += -lwinmm

# Install an application icon for Windows to use.

win32:RC_ICONS += ../resources/icons/route66.ico \
 ../resources/icons/route66-16x16.ico \
 ../resources/icons/route66-32x32.ico \
 ../resources/icons/route66-48x48.ico \
 ../resources/icons/route66-64x64.ico \
 ../resources/icons/route66-256x256.ico
windows:RC_ICONS += ../resources/icons/route66.ico \
 ../resources/icons/route66-16x16.ico \
 ../resources/icons/route66-32x32.ico \
 ../resources/icons/route66-48x48.ico \
 ../resources/icons/route66-64x64.ico \
 ../resources/icons/route66-256x256.ico

#******************************************************************************
# Seq66qt5.pro (Seq66qt5)
#------------------------------------------------------------------------------
# 	vim: ts=3 sw=3 ft=automake
#------------------------------------------------------------------------------
