#!/bin/bash
#
# From
#
# https://archived.forum.manjaro.org/t/how-to-replace-pulseaudio-with-
#       jack-jack-and-pulseaudio-together-as-friend/2086
#

PULSESINKID=$(pactl list | grep -B 1 "Name: module-jack-sink" | grep Module | sed 's/[^0-9]//g')
PULSESOURCEID=$(pactl list | grep -B 1 "Name: module-jack-source" | grep Module | sed 's/[^0-9]//g')

pactl unload-module $PULSESINKID             # module-jack-sink
pactl unload-module $PULSESOURCEID           # module-jack-source
sleep 5

# vim: ts=3
