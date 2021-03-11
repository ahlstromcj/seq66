#!/bin/sh
#
# Additional options:
#
#   --suppressions=contrib/valgrind/seq66.supp
#   --leak-resolution=high

valgrind --tool=helgrind --log-file=helgrind.log $*

# vim: ts=3 sw=3 wm=4 et ft=sh
