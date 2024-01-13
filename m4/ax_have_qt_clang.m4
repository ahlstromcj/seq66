# ===========================================================================
# https://www.gnu.org/software/autoconf-archive/ax_have_qt_clang.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_HAVE_QT_CLANG
#
# DESCRIPTION
#
#   Modified by C. Ahlstrom 2023-12-08 to remove the paranoid testing
#   of Qt when using Clang-16.
#
#   A modification of ax_have_qt_min.m4.
#
# ===========================================================================

#serial 19

AU_ALIAS([BNV_HAVE_QT], [AX_HAVE_QT_CLANG])
AC_DEFUN([AX_HAVE_QT_CLANG],
[
  AC_REQUIRE([AC_PROG_CXX])
  AC_REQUIRE([AC_PATH_X])
  AC_REQUIRE([AC_PATH_XTRA])

  AC_MSG_CHECKING(for Qt)

  # From serial 19 to handle issue #54.
  #
  # openSUSE leap 15.3 installs qmake-qt5, not qmake, for example.
  # Store the full name (like qmake-qt5) into am_have_qt_qmexe
  # and the specifier (like -qt5 or empty) into am_have_qt_qmexe_suff.

  AC_CHECK_PROGS(am_have_qt_qmexe,qmake qmake-qt6 qmake-qt5,[])
  am_have_qt_qmexe_suff=`echo $am_have_qt_qmexe | cut -b 6-`
  # If we have Qt5 or later in the path, we're golden
  ver=`$am_have_qt_qmexe --version | grep -o "Qt version ."`

  if test "$ver" ">" "Qt version 4"; then
    have_qt=yes
    # This pro file dumps qmake's variables, but it only works on Qt 5 or later
    am_have_qt_dir=`mktemp -d`
    am_have_qt_pro="$am_have_qt_dir/test.pro"
    am_have_qt_makefile="$am_have_qt_dir/Makefile"
    # http://qt-project.org/doc/qt-5/qmake-variable-reference.html#qt
    cat > $am_have_qt_pro << EOF
win32 {
    CONFIG -= debug_and_release
    CONFIG += release
}
qtHaveModule(core):              QT += core
qtHaveModule(gui):               QT += gui
qtHaveModule(widgets):           QT += widgets


percent.target = %
percent.commands = @echo -n "\$(\$(@))\ "
QMAKE_EXTRA_TARGETS += percent
EOF
    $am_have_qt_qmexe $am_have_qt_pro -o $am_have_qt_makefile
    echo "QT TEST Makefile = $am_have_qt_makefile"

    # Get Qt version from qmake

    QT_CXXFLAGS=`cd $am_have_qt_dir; make -s -f $am_have_qt_makefile CXXFLAGS INCPATH`
    QT_LIBS=`cd $am_have_qt_dir; make -s -f $am_have_qt_makefile LIBS`
    rm $am_have_qt_pro $am_have_qt_makefile
    rmdir $am_have_qt_dir

    # Look for specific tools in $PATH

    QT_MOC=`which moc$am_have_qt_qmexe_suff`
    QT_UIC=`which uic$am_have_qt_qmexe_suff`
    QT_RCC=`which rcc$am_have_qt_qmexe_suff`
    QT_LRELEASE=`which lrelease$am_have_qt_qmexe_suff`
    QT_LUPDATE=`which lupdate$am_have_qt_qmexe_suff`

    # Get Qt version from qmake

    QT_DIR=`$am_have_qt_qmexe --version | grep -o -E /.+`

    # All variables are defined, report the result

    AC_MSG_RESULT([$have_qt:
    QT_CXXFLAGS=$QT_CXXFLAGS
    QT_DIR=$QT_DIR
    QT_LIBS=$QT_LIBS
    QT_UIC=$QT_UIC
    QT_MOC=$QT_MOC
    QT_RCC=$QT_RCC
    QT_LRELEASE=$QT_LRELEASE
    QT_LUPDATE=$QT_LUPDATE])
  else

    # Qt was not found

    have_qt=no
    QT_CXXFLAGS=
    QT_DIR=
    QT_LIBS=
    QT_UIC=
    QT_MOC=
    QT_RCC=
    QT_LRELEASE=
    QT_LUPDATE=
    AC_MSG_RESULT($have_qt)
    AC_MSG_ERROR(qmake/Qt not found... is any qmake on your system?)
  fi
  AC_SUBST(QT_CXXFLAGS)
  AC_SUBST(QT_DIR)
  AC_SUBST(QT_LIBS)
  AC_SUBST(QT_UIC)
  AC_SUBST(QT_MOC)
  AC_SUBST(QT_RCC)
  AC_SUBST(QT_LRELEASE)
  AC_SUBST(QT_LUPDATE)

])
