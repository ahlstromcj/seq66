#******************************************************************************
# libseq66.pro (qpseq66)
#------------------------------------------------------------------------------
##
# \file        libseq66.pro
# \library     qseq66 and qpseq66 application
# \author      Chris Ahlstrom
# \date        2018-11-15
# \update      2021-01-22
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

HEADERS += include/app_limits.h \
 include/seq66_features.h \
 include/seq66_features.hpp \
 include/seq66_platform_macros.h \
 include/cfg/basesettings.hpp \
 include/cfg/cmdlineopts.hpp \
 include/cfg/comments.hpp \
 include/cfg/configfile.hpp \
 include/cfg/midicontrolfile.hpp \
 include/cfg/mutegroupsfile.hpp \
 include/cfg/notemapfile.hpp \
 include/cfg/playlistfile.hpp \
 include/cfg/rcfile.hpp \
 include/cfg/rcsettings.hpp \
 include/cfg/recent.hpp \
 include/cfg/scales.hpp \
 include/cfg/settings.hpp \
 include/cfg/userinstrument.hpp \
 include/cfg/usermidibus.hpp \
 include/cfg/usrfile.hpp \
 include/cfg/usrsettings.hpp \
 include/ctrl/automation.hpp \
 include/ctrl/keycontrol.hpp \
 include/ctrl/keycontainer.hpp \
 include/ctrl/keymap.hpp \
 include/ctrl/keystroke.hpp \
 include/ctrl/midicontrolin.hpp \
 include/ctrl/midicontrolbase.hpp \
 include/ctrl/midicontrol.hpp \
 include/ctrl/midicontrolout.hpp \
 include/ctrl/midioperation.hpp \
 include/ctrl/opcontainer.hpp \
 include/ctrl/opcontrol.hpp \
 include/midi/businfo.hpp \
 include/midi/controllers.hpp \
 include/midi/editable_event.hpp \
 include/midi/editable_events.hpp \
 include/midi/event.hpp \
 include/midi/eventlist.hpp \
 include/midi/jack_assistant.hpp \
 include/midi/mastermidibase.hpp \
 include/midi/midibase.hpp \
 include/midi/midibytes.hpp \
 include/midi/midifile.hpp \
 include/midi/midi_splitter.hpp \
 include/midi/midi_vector_base.hpp \
 include/midi/midi_vector.hpp \
 include/midi/songsummary.hpp \
 include/midi/wrkfile.hpp \
 include/play/clockslist.hpp \
 include/play/inputslist.hpp \
 include/play/listsbase.hpp \
 include/play/mutegroup.hpp \
 include/play/mutegroups.hpp \
 include/play/notemapper.hpp \
 include/play/performer.hpp \
 include/play/playlist.hpp \
 include/play/screenset.hpp \
 include/play/seq.hpp \
 include/play/sequence.hpp \
 include/play/setmapper.hpp \
 include/play/setmaster.hpp \
 include/play/triggers.hpp \
 include/qt/qsmacros.hpp \
 include/sessions/clinsmanager.hpp \
 include/sessions/smanager.hpp \
 include/os/daemonize.hpp \
 include/os/timing.hpp \
 include/util/automutex.hpp \
 include/util/basic_macros.h \
 include/util/basic_macros.hpp \
 include/util/calculations.hpp \
 include/util/condition.hpp \
 include/util/filefunctions.hpp \
 include/util/palette.hpp \
 include/util/recmutex.hpp \
 include/util/rect.hpp \
 include/util/strfunctions.hpp \
 include/util/victor.hpp

SOURCES += src/seq66_features.cpp \
 src/cfg/basesettings.cpp \
 src/cfg/cmdlineopts.cpp \
 src/cfg/comments.cpp \
 src/cfg/configfile.cpp \
 src/cfg/midicontrolfile.cpp \
 src/cfg/mutegroupsfile.cpp \
 src/cfg/notemapfile.cpp \
 src/cfg/playlistfile.cpp \
 src/cfg/rcfile.cpp \
 src/cfg/rcsettings.cpp \
 src/cfg/recent.cpp \
 src/cfg/scales.cpp \
 src/cfg/settings.cpp \
 src/cfg/userinstrument.cpp \
 src/cfg/usermidibus.cpp \
 src/cfg/usrfile.cpp \
 src/cfg/usrsettings.cpp \
 src/ctrl/automation.cpp \
 src/ctrl/keycontainer.cpp \
 src/ctrl/keycontrol.cpp \
 src/ctrl/keymap.cpp \
 src/ctrl/keystroke.cpp \
 src/ctrl/midicontrolin.cpp \
 src/ctrl/midicontrolbase.cpp \
 src/ctrl/midicontrol.cpp \
 src/ctrl/midicontrolout.cpp \
 src/ctrl/midioperation.cpp \
 src/ctrl/opcontainer.cpp \
 src/ctrl/opcontrol.cpp \
 src/midi/businfo.cpp \
 src/midi/controllers.cpp \
 src/midi/editable_event.cpp \
 src/midi/editable_events.cpp \
 src/midi/event.cpp \
 src/midi/eventlist.cpp \
 src/midi/jack_assistant.cpp \
 src/midi/mastermidibase.cpp \
 src/midi/midibase.cpp \
 src/midi/midibytes.cpp \
 src/midi/midifile.cpp \
 src/midi/midi_splitter.cpp \
 src/midi/midi_vector_base.cpp \
 src/midi/midi_vector.cpp \
 src/midi/songsummary.cpp \
 src/midi/wrkfile.cpp \
 src/play/clockslist.cpp \
 src/play/inputslist.cpp \
 src/play/listsbase.cpp \
 src/play/mutegroup.cpp \
 src/play/mutegroups.cpp \
 src/play/notemapper.cpp \
 src/play/performer.cpp \
 src/play/playlist.cpp \
 src/play/screenset.cpp \
 src/play/seq.cpp \
 src/play/sequence.cpp \
 src/play/setmapper.cpp \
 src/play/setmaster.cpp \
 src/play/triggers.cpp \
 src/sessions/clinsmanager.cpp \
 src/sessions/smanager.cpp \
 src/os/daemonize.cpp \
 src/os/timing.cpp \
 src/util/automutex.cpp \
 src/util/basic_macros.cpp \
 src/util/calculations.cpp \
 src/util/condition.cpp \
 src/util/filefunctions.cpp \
 src/util/palette.cpp \
 src/util/recmutex.cpp \
 src/util/rect.cpp \
 src/util/strfunctions.cpp

INCLUDEPATH = \
 ../include/qt/$${MIDILIB} \
 ../libsessions/include \
 ../seq_$${MIDILIB}/include \
 include

#******************************************************************************
# libseq66.pro (qpseq66)
#------------------------------------------------------------------------------
# vim: ts=4 sw=4 ft=automake
#------------------------------------------------------------------------------
