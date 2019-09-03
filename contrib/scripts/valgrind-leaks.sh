#!/bin/sh
#
# Additional options:
#
#   --suppressions=contrib/valgrind/seq64.supp
#   --leak-resolution=high

valgrind --leak-check=full --track-origins=yes --log-file=valgrind.log \
 --show-leak-kinds=all $*

