#!/bin/bash
#
# From
#
# https://archived.forum.manjaro.org/t/how-to-replace-pulseaudio-with-
#       jack-jack-and-pulseaudio-together-as-friend/2086
#
# For some reason, putting the load-module commands in /etc/pulse/default.pa
# does not work.

pactl load-module module-jack-sink
pactl load-module module-jack-source

pacmd set-default-sink jack_out
pacmd set-default-source jack_in

# vim: ts=3
