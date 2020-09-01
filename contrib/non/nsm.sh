#!/bin/sh
#------------------------------------------------------------------------------
##
# \file        nsm.sh
# \library     contrib/non
# \author      Chris Ahlstrom
# \date        2020-03-05 to 2020-09-01
# \version     $Revision$
# \license     GNU GPLv2 or above, or more generous
#
# Invoke as:
#
#   nsm.sh projectname
#
# To consider:
#
#   non-session-manager -- --osc-port 7777
#   sleep 4
#   oscsend localhost 7777 /nsm/server/open s "$1"
#
#------------------------------------------------------------------------------

PROJECT=$1  #for better readability

# The following command returns a line to provide the NSM_URL, which we use to
# set NSM_URL.  For example:
#
#       M_URL=osc.udp://mlsleno:12885/

eval $(nsmd --detach)

# Note that "nsdm --help" leaves out some options that available. Here are
# all the options:
#
#   --detach:       Detach from console and fork a new process.
#   --session-root: Set the session path.  Defaults to "$HOME/NSM Sessions".
#   --osc-port:     Provides the UDP port number. Otherwise, it is random.
#   --gui-url:      Connects to the GUI here.  Sends the message
#                   "/nsm/gui/server_announce".
#

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
