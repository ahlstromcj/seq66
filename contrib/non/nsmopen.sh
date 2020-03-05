#!/bin/sh
#
# Invoke as:
#
#   nsmopen.sh projectname

PROJECT=$1  #for better readability

# The following command returns a line to provide the NSM_URL, which we use to
# set NSM_URL.

eval $(nsmd --detach)

# Keep in brackets so that PID and nsm are in the same group. Otherwise,
# the wait below will complain.  Remember to check (the PID) when the
# GUI closes.

{
	non-session-manager --nsm-url $NSM_URL &
	PID=$!
}

# Leave NSM time to start up. Otherwise, one will see some glitched
# session display.

sleep 1

# oscsend is part of liblo, which nsm depends on. On Ubuntu, one must install
# the liblo-tools package to get this command. It sends OpenSound Control
# messages via UDP.
#
# Usage:
#
#       oscsend hostname port address types values ...
#       oscsend url address types values ...
#
# Note: There is also a sendosc project on GitHub.

oscsend $NSM_URL "/nsm/server/open" s $PROJECT
wait $PID
echo "GUI closed. Closing detached nsdm server."
oscsend $NSM_URL "/nsm/server/quit"

# vim: sw=4 ts=4 wm=8 et ft=sh
