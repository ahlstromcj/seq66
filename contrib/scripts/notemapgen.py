#!/usr/bin/python3
#
#*****************************************************************************
##
#   \file           notemapgen.py
#   \author         Chris Ahlstrom
#   \updates        2025-01-10
#   \version        $Revision$
#
#   Generates a simple notemap for testing.
#
#*****************************************************************************

# Python modules

import os
import string
import sys
            
#*****************************************************************************
# __main__
#-----------------------------------------------------------------------------

file = open("test.notemap", "w")
file.write("Test notemap file: " + file.name + "\n\n")

for x in range(12, 128) :

    file.write("[Drum " + str(x) + "]\n\n")
    file.write("dev-name = \"Dev note " + str(x) + "\"\n")
    file.write("gm-name = \"GM note " + str(x - 12) + "\"\n")
    file.write("dev-note = " + str(x) + "\n")
    file.write("gm-note = " + str(x - 12) + "\n\n")


file.write("# vim: sw=4 ts=4 wm=4 et ft=dosini")
file.close()

#*****************************************************************************
# End of notemapgen.py
#-----------------------------------------------------------------------------
# vim: ts=4 sw=4 et ft=python
#-----------------------------------------------------------------------------

