#!/usr/bin/sed -i -f
#
## \file          midi_control.hpp
#
#     Quick fixes to some stock changes in going from seq64 to seq66.
# 
#  \library       sequencer66 application
#  \author        Chris Ahlstrom
#  \date          2018-11-09
#  \updates       2019-03-09
#  \license       GNU GPLv2 or above
#
#  Execution:
#
#     sed -i -f seq66.sed *.cpp *.c *.h *.hpp
#
# Order is important, the longer matches should be hit first.

s/seq24\/sequencer64/seq66/g
s/seq24/seq66/g
s/seq64/seq66/g
s/SEQ64/SEQ66/g
s/sequencer64/seq66/g
s/Sequencer64/Seq66/g
s/SEQUENCER64/SEQ66/g
s/Seq24 team; modifications by //
# s/seq66\/seq66/seq66/g

# File-name changes

s/"easy_macros.h"   /"basic_macros.hpp"/g
s/"basic_macros.h"  /"basic_macros.hpp"/g
s/CharVector/bytes/g
s/easy_macros/basic_macros/g
s/EASY_MACROS/BASIC_MACROS/g
s/EventStack/eventstack/g
s/file_functions/filefunctions/g
s/FILE_FUNCTIONS/FILEFUNCTIONS/g
s/midibyte\.hpp/midibytes.hpp/g
s/MIDIBYTE/MIDIBYTES/g
s/midi_container/midi_vector_base/g
s/MIDI_CONTAINER/MIDI_VECTOR_BASE/g
s/optionsfile/rcfile/g
s/OPTIONSFILE/RCFILE/g
s/palette_pair_t/pair/g
s/perform/performer/g
s/PERFORM/PERFORMER/g
s/performerance/performance/g
s/platform_macros/seq66_platform_macros/g
s/ PLATFORM_/ SEQ66_PLATFORM_/g
s/rc_settings/rcsettings/g
s/RC_SETTINGS/RCSETTINGS/g
s/SysexContainer/sysex/g
s/userfile/usrfile/g
s/USERFILE/USRFILE/g
s/user_instrument/usrinstrument/g
s/USER_INSTRUMENT/USRINSTRUMENT/g
s/user_midi_bus/usrmidibus/g
s/USER_MIDI_BUS/USRMIDIBUS/g
s/user_settings/usrsettings/g
s/USERSETTINGS/USRSETTINGS/g

#-------------------------------------------------------------------------------
# Shorten long lines having nothing but spaces, that end in *
#-------------------------------------------------------------------------------
# s/      *\*$/ */
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# End of seq66.sed
#-------------------------------------------------------------------------------
# vim: ts=3 sw=3 et ft=sed
#-------------------------------------------------------------------------------
