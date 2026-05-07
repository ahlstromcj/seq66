#*****************************************************************************
# meson.build (seq66)
#-----------------------------------------------------------------------------
##
# \file        resources/meson.build
# \library     seq66
# \author      Chris Ahlstrom
# \date        2026-04-28
# \updates     2026-05-07
# \license     $XPC_SUITE_GPL_LICENSE$
#
#  This file is part of the "seq66" library. See the top-level meson.build
#  file for license information.
#
#  Moved to the parent directory.
#
#-----------------------------------------------------------------------------

pixmaps_xpm = files(
    'bus.xpm',
    'chord3-inv.xpm',
    'chord3.xpm',
    'chord.xpm',
    'collapse.xpm',
    'copy.xpm',
    'del.xpm',
    'down.xpm',
    'drum.xpm',
    'expandgrid.xpm',
    'expand.xpm',
    'exp_rec_on.xpm',
    'filter_off.xpm',
    'filter_on.xpm',
    'finger.xpm',
    'follow.xpm',
    'fruity.xpm',
    'hex_off.xpm',
    'hex_on.xpm',
    'hide.xpm',
    'ins.xpm',
    'jack_black.xpm',
    'jack.xpm',
    'key.xpm',
    'learn2.xpm',
    'learn.xpm',
    'length_red.xpm',
    'length_short_inv.xpm',
    'length_short.xpm',
    'length.xpm',
    'live_mode.xpm',
    'logo.xpm',
    'loop.xpm',
    'menu_empty_inv.xpm',
    'menu_empty.xpm',
    'menu_full_inv.xpm',
    'menu_full.xpm',
    'menu.xpm',
    'metro_on.xpm',
    'metro.xpm',
    'midi.xpm',
    'muting.xpm',
    'note_length_inv.xpm',
    'note_length.xpm',
    'n_rec_on.xpm',
    'numbers_off.xpm',
    'numbers_on.xpm',
    'panic2.xpm',
    'panic.xpm',
    'pause.xpm',
    'perfedit.xpm',
    'play2.xpm',
    'play_on.xpm',
    'play.xpm',
    'q_rec_on.xpm',
    'q_rec.xpm',
    'qseq66.png',
    'quantize_inv.xpm',
    'quantize.xpm',
    'rec_ex_buss.xpm',
    'rec_ex_channel.xpm',
    'rec_ex_normal.xpm',
    'rec_on.xpm',
    'rec.xpm',
    'redo.xpm',
    'right.xpm',
    'route66rwb-32x32.xpm',
    'route66rwb-64x64.xpm',
    'route66.xpm',
    'scale.xpm',
    'seq66_32.xpm',
    'seq66.xpm',
    'seq-editor.xpm',
    'sequences.xpm',
    'seq.xpm',
    'show_bars_off.xpm',
    'show_bars_on.xpm',
    'show.xpm',
    'snap.xpm',
    'song-editor.xpm',
    'song_mode.xpm',
    'song_rec_no_snap.xpm',
    'song_rec_off.xpm',
    'song_rec_on.xpm',
    'song_record.xpm',
    'song_rec.xpm',
    'song-snap.xpm',
    'stop.xpm',
    'thru_on.xpm',
    'thru.xpm',
    'tools.xpm',
    'transport_follow.xpm',
    'transpose.xpm',
    't_rec_on.xpm',
    'tux.xpm',
    'undo.xpm',
    'up_inv.xpm',
    'up.xpm',
    'zoom_in.xpm',
    'zoom_out.xpm',
    'zoom.xpm',
    )

#-----------------------------------------------------------------------------
# This loop copies each file to the directory 'build/resources/pixmaps'.
#-----------------------------------------------------------------------------

foreach xpm : pixmaps_xpm

   # message('Processing XPM: ' + '@0@'.format(xpm))

   fs.copyfile(xpm)

endforeach

#****************************************************************************
# meson.build (resources)
#----------------------------------------------------------------------------
# vim: ts=3 sw=3 ft=meson
#----------------------------------------------------------------------------
