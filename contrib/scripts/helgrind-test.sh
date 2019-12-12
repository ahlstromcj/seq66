#!/bin/sh
#
# Additional options:
#
#   --suppressions=contrib/valgrind/seq66.supp
#   --leak-resolution=high

valgrind --tool=helgrind --log-file=helgrind.log $*

