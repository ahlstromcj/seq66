#if ! defined SEQ66_GDK_BASIC_KEYS_HPP
#define SEQ66_GDK_BASIC_KEYS_HPP

/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  seq66 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with seq66; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          gdk_basic_keys.hpp
 *
 *  This module defines a non-Unicode subset of the keys defined by Gtk-2/3.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2015-09-13
 * \updates       2018-12-09
 * \license       GNU GPLv2 or above
 *
 *  This file is provided as a convenience so that we have some reasonable
 *  and universal set of basic PC keys to use in Seq64-to-Seq66 conversions.
 *  It is a cut-down version of the libseq6seq66lude/gdk_basic_keys.h file.
 *  Shamelessly derived from the following files:
 *
 *      /usr/include/gtk-2.0/gdk/gdkkeysyms-compat.h
 *      /usr/include/gtk-2.0/gdk/gdkevents.h
 */

/**
 *  Defines our own names for keystrokes, so that we don't need to rely
 *  on the headers of a particular user-interface framework.
 *  We might include the official header file instead at some point, once this
 *  refactoring settles down.
 */

#if ! defined SEQ66_Home

#define SEQ66_BackSpace           0xff08
#define SEQ66_Tab                 0xff09
#define SEQ66_Linefeed            0xff0a
#define SEQ66_Clear               0xff0b    /* in Qt 5, a musical note! */
#define SEQ66_Return              0xff0d
#define SEQ66_Pause               0xff13
#define SEQ66_Scroll_Lock         0xff14
#define SEQ66_Sys_Req             0xff15
#define SEQ66_Escape              0xff1b
#define SEQ66_Delete              0xffff
#define SEQ66_Home                0xff50
#define SEQ66_Left                0xff51
#define SEQ66_Up                  0xff52
#define SEQ66_Right               0xff53
#define SEQ66_Down                0xff54
#define SEQ66_Prior               0xff55
#define SEQ66_Page_Up             0xff55
#define SEQ66_Next                0xff56
#define SEQ66_Page_Down           0xff56
#define SEQ66_End                 0xff57
#define SEQ66_Begin               0xff58
#define SEQ66_Select              0xff60
#define SEQ66_Print               0xff61
#define SEQ66_Execute             0xff62
#define SEQ66_Insert              0xff63
#define SEQ66_Undo                0xff65
#define SEQ66_Redo                0xff66
#define SEQ66_Menu                0xff67
#define SEQ66_Find                0xff68
#define SEQ66_Cancel              0xff69
#define SEQ66_Help                0xff6a
#define SEQ66_Break               0xff6b
#define SEQ66_Mode_switch         0xff7e
#define SEQ66_script_switch       0xff7e
#define SEQ66_Num_Lock            0xff7f

#define SEQ66_F1                  0xffbe
#define SEQ66_F2                  0xffbf
#define SEQ66_F3                  0xffc0
#define SEQ66_F4                  0xffc1
#define SEQ66_F5                  0xffc2
#define SEQ66_F6                  0xffc3
#define SEQ66_F7                  0xffc4
#define SEQ66_F8                  0xffc5
#define SEQ66_F9                  0xffc6
#define SEQ66_F10                 0xffc7
#define SEQ66_F11                 0xffc8
#define SEQ66_F12                 0xffc9

#define SEQ66_KP_Space            0xff80
#define SEQ66_KP_Tab              0xff89
#define SEQ66_KP_Enter            0xff8d
#define SEQ66_KP_F1               0xff91
#define SEQ66_KP_F2               0xff92
#define SEQ66_KP_F3               0xff93
#define SEQ66_KP_F4               0xff94
#define SEQ66_KP_Home             0xff95
#define SEQ66_KP_Left             0xff96
#define SEQ66_KP_Up               0xff97
#define SEQ66_KP_Right            0xff98
#define SEQ66_KP_Down             0xff99
#define SEQ66_KP_Prior            0xff9a
#define SEQ66_KP_Page_Up          0xff9a
#define SEQ66_KP_Next             0xff9b
#define SEQ66_KP_Page_Down        0xff9b
#define SEQ66_KP_End              0xff9c
#define SEQ66_KP_Begin            0xff9d
#define SEQ66_KP_Insert           0xff9e
#define SEQ66_KP_Delete           0xff9f
#define SEQ66_KP_Equal            0xffbd
#define SEQ66_KP_Multiply         0xffaa
#define SEQ66_KP_Add              0xffab
#define SEQ66_KP_Separator        0xffac
#define SEQ66_KP_Subtract         0xffad
#define SEQ66_KP_Decimal          0xffae
#define SEQ66_KP_Divide           0xffaf
#define SEQ66_KP_0                0xffb0
#define SEQ66_KP_1                0xffb1
#define SEQ66_KP_2                0xffb2
#define SEQ66_KP_3                0xffb3
#define SEQ66_KP_4                0xffb4
#define SEQ66_KP_5                0xffb5
#define SEQ66_KP_6                0xffb6
#define SEQ66_KP_7                0xffb7
#define SEQ66_KP_8                0xffb8
#define SEQ66_KP_9                0xffb9

#define SEQ66_Shift_L             0xffe1
#define SEQ66_Shift_R             0xffe2
#define SEQ66_Control_L           0xffe3
#define SEQ66_Control_R           0xffe4
#define SEQ66_Caps_Lock           0xffe5
#define SEQ66_Shift_Lock          0xffe6
#define SEQ66_Meta_L              0xffe7
#define SEQ66_Meta_R              0xffe8
#define SEQ66_Alt_L               0xffe9
#define SEQ66_Alt_R               0xffea
#define SEQ66_Super_L             0xffeb
#define SEQ66_Super_R             0xffec
#define SEQ66_Hyper_L             0xffed
#define SEQ66_Hyper_R             0xffee
#define SEQ66_ISO_Lock            0xfe01
#define SEQ66_space               0x020
#define SEQ66_exclam              0x021
#define SEQ66_quotedbl            0x022
#define SEQ66_numbersign          0x023
#define SEQ66_dollar              0x024
#define SEQ66_percent             0x025
#define SEQ66_ampersand           0x026
#define SEQ66_apostrophe          0x027
#define SEQ66_quoteright          0x027
#define SEQ66_parenleft           0x028
#define SEQ66_parenright          0x029
#define SEQ66_asterisk            0x02a
#define SEQ66_plus                0x02b
#define SEQ66_comma               0x02c
#define SEQ66_minus               0x02d
#define SEQ66_period              0x02e
#define SEQ66_slash               0x02f
#define SEQ66_0                   0x030
#define SEQ66_1                   0x031
#define SEQ66_2                   0x032
#define SEQ66_3                   0x033
#define SEQ66_4                   0x034
#define SEQ66_5                   0x035
#define SEQ66_6                   0x036
#define SEQ66_7                   0x037
#define SEQ66_8                   0x038
#define SEQ66_9                   0x039
#define SEQ66_colon               0x03a
#define SEQ66_semicolon           0x03b
#define SEQ66_less                0x03c
#define SEQ66_equal               0x03d
#define SEQ66_greater             0x03e
#define SEQ66_question            0x03f
#define SEQ66_at                  0x040
#define SEQ66_A                   0x041
#define SEQ66_B                   0x042
#define SEQ66_C                   0x043
#define SEQ66_D                   0x044
#define SEQ66_E                   0x045
#define SEQ66_F                   0x046
#define SEQ66_G                   0x047
#define SEQ66_H                   0x048
#define SEQ66_I                   0x049
#define SEQ66_J                   0x04a
#define SEQ66_K                   0x04b
#define SEQ66_L                   0x04c
#define SEQ66_M                   0x04d
#define SEQ66_N                   0x04e
#define SEQ66_O                   0x04f
#define SEQ66_P                   0x050
#define SEQ66_Q                   0x051
#define SEQ66_R                   0x052
#define SEQ66_S                   0x053
#define SEQ66_T                   0x054
#define SEQ66_U                   0x055
#define SEQ66_V                   0x056
#define SEQ66_W                   0x057
#define SEQ66_X                   0x058
#define SEQ66_Y                   0x059
#define SEQ66_Z                   0x05a
#define SEQ66_bracketleft         0x05b
#define SEQ66_backslash           0x05c
#define SEQ66_bracketright        0x05d
#define SEQ66_asciicircum         0x05e
#define SEQ66_underscore          0x05f
#define SEQ66_grave               0x060
#define SEQ66_quoteleft           0x060
#define SEQ66_a                   0x061
#define SEQ66_b                   0x062
#define SEQ66_c                   0x063
#define SEQ66_d                   0x064
#define SEQ66_e                   0x065
#define SEQ66_f                   0x066
#define SEQ66_g                   0x067
#define SEQ66_h                   0x068
#define SEQ66_i                   0x069
#define SEQ66_j                   0x06a
#define SEQ66_k                   0x06b
#define SEQ66_l                   0x06c
#define SEQ66_m                   0x06d
#define SEQ66_n                   0x06e
#define SEQ66_o                   0x06f
#define SEQ66_p                   0x070
#define SEQ66_q                   0x071
#define SEQ66_r                   0x072
#define SEQ66_s                   0x073
#define SEQ66_t                   0x074
#define SEQ66_u                   0x075
#define SEQ66_v                   0x076
#define SEQ66_w                   0x077
#define SEQ66_x                   0x078
#define SEQ66_y                   0x079
#define SEQ66_z                   0x07a
#define SEQ66_braceleft           0x07b
#define SEQ66_bar                 0x07c
#define SEQ66_braceright          0x07d
#define SEQ66_asciitilde          0x07e
#define SEQ66_igrave              0x0ec

#endif      // SEQ66_Home

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/*
 *  Free functions in the seq66 namespace.
 */

extern std::string gdk_key_name (unsigned gdkkeycode);

}           // namespace seq66

#endif      // SEQ66_GDK_BASIC_KEYS_HPP

/*
 * gdk_basic_keys.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

