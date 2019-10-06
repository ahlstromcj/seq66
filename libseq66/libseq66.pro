#******************************************************************************
# libseq66.pro (qpseq66)
#------------------------------------------------------------------------------
##
# \file        libseq66.pro
# \library     qseq66 and qpseq66 application
# \author      Chris Ahlstrom
# \date        2018-11-15
# \update      2019-10-06
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

HEADERS += \
 include/app_limits.h \
 include/main_impl.hpp \
 include/seq66_features.h \
 include/seq66_features.hpp \
 include/seq66_platform_macros.h \
 include/cfg/basesettings.hpp \
 include/cfg/cmdlineopts.hpp \
 include/cfg/comments.hpp \
 include/cfg/configfile.hpp \
 include/cfg/midicontrolfile.hpp \
 include/cfg/mutegroupsfile.hpp \
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
 include/ctrl/midicontainer.hpp \
 include/ctrl/midicontrol.hpp \
 include/ctrl/midicontrolout.hpp \
 include/ctrl/midioperation.hpp \
 include/ctrl/opcontainer.hpp \
 include/ctrl/opcontrol.hpp \
 include/midi/event.hpp \
 include/midi/event_list.hpp \
 include/midi/midibytes.hpp \
 include/midi/midi_vector_base.hpp \
 include/midi/midi_vector.hpp \
 include/play/clockslist.hpp \
 include/play/inputslist.hpp \
 include/play/mutegroup.hpp \
 include/play/mutegroups.hpp \
 include/play/performer.hpp \
 include/play/playlist.hpp \
 include/play/screenset.hpp \
 include/play/seq.hpp \
 include/play/sequence.hpp \
 include/play/setmapper.hpp \
 include/play/triggers.hpp \
 include/qt/qsmacros.hpp \
 include/unix/daemonize.hpp \
 include/util/automutex.hpp \
 include/util/basic_macros.h \
 include/util/basic_macros.hpp \
 include/util/calculations.hpp \
 include/util/conditio.hpp \
 include/util/filefunctions.hpp \
 include/util/palette.hpp \
 include/util/recmutex.hpp \
 include/util/rect.hpp \
 include/util/strfunctions.hpp \
 include/util/victor.hpp

SOURCES += \
 src/main_impl.cpp \
 src/seq66_features.cpp \
 src/cfg/basesettings.cpp \
 src/cfg/cmdlineopts.cpp \
 src/cfg/comments.cpp \
 src/cfg/configfile.cpp \
 src/cfg/midicontrolfile.cpp \
 src/cfg/mutegroupsfile.cpp \
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
 src/ctrl/midicontainer.cpp \
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
 src/midi/event_list.cpp \
 src/midi/jack_assistant.cpp \
 src/midi/mastermidibase.cpp \
 src/midi/midibase.cpp \
 src/midi/midibytes.cpp \
 src/midi/midifile.cpp \
 src/midi/midi_splitter.cpp \
 src/midi/midi_vector_base.cpp \
 src/midi/midi_vector.cpp \
 src/midi/wrkfile.cpp \
 src/play/mutegroup.cpp \
 src/play/mutegroups.cpp \
 src/play/performer.cpp \
 src/play/playlist.cpp \
 src/play/screenset.cpp \
 src/play/seq.cpp \
 src/play/sequence.cpp \
 src/play/setmapper.cpp \
 src/play/triggers.cpp \
 src/unix/daemonize.cpp \
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
 ../include/qt/portmidi \
 ../seq_portmidi/include \
 include

#******************************************************************************
# libseq66.pro (qpseq66)
#------------------------------------------------------------------------------
# vim: ts=4 sw=4 ft=automake
#------------------------------------------------------------------------------
