#!/bin/sh
#
# Additional options:
#
#   --suppressions=contrib/valgrind/seq64.supp
#   --leak-resolution=high

valgrind --tool=helgrind --log-file=helgrind.log $*

