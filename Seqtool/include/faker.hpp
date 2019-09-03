#if ! defined FAKER_HPP
#define FAKER_HPP

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
 * \file          faker.hpp
 * \library       Seqtool (from the Seq66 project)
 * \author        Chris Ahlstrom
 * \date          2018-11-18
 * \updates       2018-11-25
 * \version       $Revision$
 * \license       $XPC_SUITE_GPL_LICENSE$
 *
 *    This module provides a test case to help us learn how to add member
 *    functions and "member" lambda functions to a midioperation object.
 */

#include <memory>                       /* std::unique_ptr<>                */

#include "ctrl/keycontainer.hpp"        /* class seq66::keycontainer        */
#include "ctrl/midicontainer.hpp"       /* class seq66::keycontainer        */
#include "ctrl/opcontainer.hpp"         /* class seq66::opcontainer         */

/**
 *  Our fake performer class.
 */

class faker
{

private:

    /**
     *  Defines a pointer to a member automation function.
     */

    using automation_function = bool (faker::*)
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );

    /**
     * Defines a structure of slot/function pairs, used to create an array used
     * to initialize the automation callbacks.
     */

    using automation_pair = struct
    {
        seq66::automation::slot ap_slot;
        automation_function ap_function;
    };

    /**
     *  An array of slot/function pairs, used to initialize the automation
     *  callbacks.
     */

    static automation_pair sm_auto_func_list[];

    /**
     *  Used only for working out function issues.
     */

    std::unique_ptr<seq66::midioperation> m_scratch_op_ptr;

    /**
     *  Provides a default-filled keycontrol container.
     */

    seq66::keycontainer m_keycontrols;

    /**
     *  Provides a default-filled midicontrol container.
     */

    seq66::midicontainer m_midicontrols;

    /**
     *  Used for testing the populating of an opcontainer.
     */

    seq66::opcontainer m_operations;

public:

    faker ();
    ~faker ();

    void create_static_op ();
    void create_member_op ();
    void create_lambda_op ();

    bool handle_keystroke (seq66::ctrlkey key);

private:

    static void print_parameters
    (
        const std::string & tag,
        seq66::automation::action a,
        int d0, int d1, bool inverse
    );
    static bool static_midi_op
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool member_midi_op
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool pattern_control
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool mute_group_control
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    void show_ordinal_error (seq66::ctrlkey ordinal, const std::string & tag);
    bool populate_default_ops ();
    bool add_automation
    (
        seq66::automation::slot s,
        automation_function f
    );
    std::string action_name (seq66::automation::action a);
    bool automation_no_op
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_bpm_up_dn
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_bpm_dn
    (
        seq66::automation::action a,int d0, int d1, bool inverse
    );
    bool automation_ss_up_dn
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_ss_dn
    (
        seq66::automation::action a,int d0, int d1, bool inverse
    );
    bool automation_replace
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_snapshot
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_queue
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_gmute
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_glearn
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_play_ss
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_playback
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_song_record
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_solo
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_thru
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_bpm_page_up_dn
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_bpm_page_dn
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_ss_set
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_record
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_quan_record
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_reset_seq
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_oneshot
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_FF
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_rewind
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_top
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_playlist
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_playlist_song
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_start
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_stop
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_snapshot_2
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_toggle_mutes
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_song_pointer
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );
    bool automation_keep_queue
    (
        seq66::automation::action a, int d0, int d1, bool inverse
    );

};              // class faker

#endif          // FAKER_HPP

/*
 * faker.hpp
 *
 * vim: ts=4 sw=4 et ft=cpp
 */
