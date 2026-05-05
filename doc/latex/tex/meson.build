#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        seq_qt6/doc/latex/tex/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-05-04
# \updates     2026-05-05
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#  Note that run_command() operates during project setup, while run_target()
#  works during the build.
#
#  The target 'makepdf' can be run by 'meson compile makepdf'.
#
#  latexmk  -g -silent -aux-directory=
#     /home/ahlstrom/Home/ca/mls/git/seq66/build/latex
#     -output-directory=/home/ahlstrom/Home/ca/mls/git/seq66/data/share/doc
#     -pdf seq66-user-manual
#
#  HOWEVER, this script is NOT used. The documentation must be made
#  manually by the command "./work.sh --pdf".
#
#  We might fix this later.
#
#-----------------------------------------------------------------------------

latex_prog = 'latexmk'
latex_dir = seq66_info_build_root / 'latex'
latex_aux_dir = '-aux-directory=' + latex_dir
latex_out_dir = seq66_info_project_root / 'data/share/doc'
latex_output = '-output-directory=' + latex_out_dir

custom_target(          # run_target(
   'makepdf',
   output : 'seq66-user-manual.pdf',
   command : [
      latex_prog,
      '-g',
      '-silent',
      latex_aux_dir,    # -aux-directory=/home/user/.../seq66/build/latex
      latex_output,     # -output-directory=/home/user/.../seq66/data/share/doc
      '-pdf',
      'seq66-user-manual'
      ]
   )
'''
success = result.returncode() == 0
if not success
   errmsg = result.stderr()               # vs stdout()
   warning(errmsg)
endif
'''

#****************************************************************************
# meson.build (seq_qt6/doc/latex/tex)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
