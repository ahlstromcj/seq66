#!/bin/bash
#
# From
#
# https://archived.forum.manjaro.org/t/how-to-replace-pulseaudio-with-
#       jack-jack-and-pulseaudio-together-as-friend/2086
#
# Put these scripts into the proper slots of Qjackctl's Scripting settings.
#
# Go to the Misc tab and check the the "Start JACK audio server on application
# startup", "Enable system tray icon", "Start minimized to system tray",
# "Enable D-Bus interface".
#
# Leave the other settings with their default states.
#
# Start JACK by pressing the Start button on the main interface of QJackCtl,
# Click the Connect button to play with connections. To save your connection
# and start it up after reboot, modify it in the Patchbay and make sure
# Activate Patchbay persistence is checked in the QJackCtl Setup. 

pacmd suspend true

# vim: ts=3
