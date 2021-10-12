#!/bin/bash
#
# From
#
# https://archived.forum.manjaro.org/t/how-to-replace-pulseaudio-with-
#       jack-jack-and-pulseaudio-together-as-friend/2086

# PULSESINKID=$(pactl list | grep -B 1 "Name: module-jack-sink" | grep Module | sed 's/[^0-9]//g')
# PULSESOURCEID=$(pactl list | grep -B 1 "Name: module-jack-source" | grep Module | sed 's/[^0-9]//g')

if [ "$PULSESINKID" != "" ] ; then
    echo "Unloading sink '$PULSESINKID' and source '$PULSESOURCEID'..."
    pactl unload-module $PULSESINKID
    pactl unload-module $PULSESOURCEID
    sleep 5
    export PULSESINKID=
    export PULSESOURCEID=
fi

# vim: ts=3
