#!/bin/sh
#
# Additional options:
#
#   --suppressions=contrib/valgrind/seq66.supp
#   --leak-resolution=high

SCRIPTDIR="../seq66/contrib/valgrind"
TOOL="--tool=helgrind"
CALLERS="--num-callers=50"
SUPPRESS="--suppressions=$SCRIPTDIR/kde.supp"
QTGLIB="export QT_NO_GLIB=1"
LOCKTRACK="--track-lockorders=no"
HELGRIND_OPTS="$TOOL $CALLERS $LOCKTRACK $SUPPRESS"

echo "$QTGLIB valgrind $HELGRIND_OPTS --log-file=helgrind-qt.log $* . . ."

valgrind $HELGRIND_OPTS --log-file=helgrind-qt.log $*

# vim: ts=3 sw=3 wm=4 et ft=sh
