#if ! defined SEQ66_MUTEGROUPS_HPP
#define SEQ66_MUTEGROUPS_HPP

/*
 *  This file is part of seq66.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  seq66 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with seq66; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file          mutegroups.hpp
 *
 *  This module declares a container for a number of optional mutegroup
 *  objects.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-01
 * \updates       2023-10-03
 * \license       GNU GPLv2 or above
 *
 *  This module is meant to support the main mute groups and the mute groups
 *  from the 'mutes' file.  For practical reasons, we hold the number of
 *  mute-groups to a constant (32) with a constant layout of 4 rows by 8
 *  columns.  This is a good upper limit for the number of mute-groups a user
 *  might want during a performance, and 4x8 fits well with the main keys
 *  (also used for pattern muting) on the center-left of the keyboard:
 *
\verbatim
        ! @ # $ % ^ & *
        Q W E R T Y U I
        A S D F G H J K
        Z X C V B N M <
\endverbatim
 */

#include <map>                          /* std::map<> for mutegroup storage */

#include "cfg/basesettings.hpp"         /* seq66::basesettings class        */
#include "play/mutegroup.hpp"           /* seq66::mutegroup stanza class    */

/*
 *  This namespace is not documented because it screws up the document
 *  processing done by Doxygen.
 */

namespace seq66
{

/**
 *  Provides a flexible container for mutegroup (mute-group) objects.
 */

class mutegroups final : public basesettings
{

public:

    /**
     *  Provides settings for muting.
     */

    enum class action
    {
        off,
        on,
        toggle,
        toggle_active,
        max
    };

    /**
     *  Provides mutually-exclusive codes for handling the reading/writing of
     *  mute-groups from the 'rc' file versus the MIDI file.  There's no GUI
     *  way to set this item yet.
     *
     *  saving::mutes: In this option, the mute groups are
     *  writtin only to the 'mutes' (formerly 'rc') file.
     *
     *  saving::midi: In this option, the mute groups are only written to
     *  the 'rc' file if the MIDI file did not contain non-zero mute groups.
     *  This option prevents the contamination of the 'mutes' mute-groups by
     *  the MIDI file's mute-groups.  We're going to make this the default
     *  option.  NEEDS FIXING!
     *
     *  saving::both: This is the legacy (seq66) option, which
     *  reads the mute-groups from the MIDI file, and saves them back to the
     *  'rc' file and to the MIDI file.  However, for Seq66 MIDI files such as
     *  b4uacuse-stress.midi, seq66 never reads the mute-groups in that MIDI
     *  file!  In any case, this can be considered a corruption of the 'rc'
     *  file.
     */

    enum class saving
    {
        mutes,              /**< Save mute groups to the 'mutes' file.      */
        midi,               /**< Write mute groups only to the MIDI file.   */
        both,               /**< Write the mute groups to both files.       */
        max                 /**< Keep this last... it is only a size value. */
    };

    /**
     *  More codes, better than booleans.  Legacy values of 'true' convert to
     *  'mutes'; 'false' converts to 'midi'.
     */

    enum class loading
    {
        none,               /**< Do not load any mute groups.               */
        mutes,              /**< Load mute groups only from 'mutes' file.   */
        midi,               /**< Load from MIDI, ignoring the 'mutes' file. */
        both,               /**< Read from 'mutes'; if none, then MIDI.     */
        max                 /**< Keep this last... it is only a size value. */
    };

public:

    /**
     *  Provides a container type for mutegroup objects, keyed by the group
     *  number.  Remember that a mutegroup object has a 2-D vector of midibools,
     *  with a given number of rows and columns, along with the group number
     *  and the number of patterns in the group.
     */

    using container = std::map<mutegroup::number, mutegroup>;

private:

    /**
     *  The virtual number of rows in a grid of mute-groups.  This number is
     *  constant in order to indicate that there are always 4 rows in the
     *  whole collection of mute-groups.  Why?  Because we can, in all
     *  practicality, only support 4 x 8 mute-groups with the keystrokes on a
     *  keyboard without interfering with other automation keys.
     */

    static const int c_rows = 4;

    /**
     *  The virtual number of columns in a grid of mute-groups.  Similarly,
     *  this item reflects that we will always have 4 x 8 mute-groups.
     */

    static const int c_columns = 8;

    /**
     *  This value indicates that there is no mute-group selected.
     */

    static const int c_null_mute_group = (-1);

    /**
     *  We force a maximum number of mute-groups.  We really only have enough
     *  keys available for 32 mute-groups.
     */

    static const int c_mute_groups_max = 32;

    /**
     *  Experiment feature to swap coordinates.
     */

    static bool s_swap_coordinates;

    /**
     *  Holds a set of mutegroup objects keyed by the configured mute
     *  group number.
     */

    container m_container;

    /**
     *  A name to use for showing the contents of the container.
     */

    std::string m_container_name;

    /**
     *  Indicates the number of rows in a group, for reading purposes.  This
     *  value defaults to 4.  A "row" in the mute-group file is demarcated by
     *  square brackets.  The concept of rows and columns is simply a device
     *  to make the mute-group file easier to read for humans by breaking one
     *  line of data in the file into smaller sections.
     *
     *  Note that each line in the mute-group file is meant to represent one
     *  and only one mutegroup object.
     */

    int m_rows;

    /**
     *  Indicates the number of columns in a group.  This value defaults to 8.
     *  A "column" in the mute-group file is one digit or bit inside the
     *  square brackets.  There are rows x column "bits" in a mute-group.
     */

    int m_columns;

    /**
     *  If true, writes the output to a mutes file in hex format.  The default
     *  is binary (0 or 1) format, but for larger mute-groups, hex form (0x00 to
     *  0xff) will save a lot of space.
     */

    bool m_group_format_hex;

    /**
     *  Indicates if the control values were loaded from an 'rc' configuration
     *  file, as opposed to being empty.  The default value is false.
     */

    bool m_loaded_from_mutes;

    /**
     *  Indicates that a mute-group-related key has just been pressed, or a
     *  similar event (MIDI or the "L" button) has occurred.
     */

    bool m_group_event;

    /**
     *  Indicates that an error occurred in group processing.  The caller will
     *  check this flag, which clears it, and act on the status.
     */

    mutable bool m_group_error;

    /**
     *  If true, indicates that a mode group is selected, and playing statuses
     *  will be "memorized".  This value starts out true.  It is altered by
     *  the MIDI control group-mute handler or when the group-off or group-on
     *  keys are struck.
     */

    bool m_group_mode;

    /**
     *  If true, indicates that a group learn is selected, which also
     *  "memorizes" a mode group, and notifies subscribers of a group-learn
     *  change.
     */

    bool m_group_learn;

    /**
     *  Selects a group to mute.  A "group" is essentially a "set" that is
     *  selected for the saving and restoring of the status of all patterns in
     *  that set.  The value of -1 (SEQ66_NO_MUTE_GROUP_SELECTED) to indicate
     *  the value should not be used.  The test for a valid value is simple,
     *  just check for group >= 0 and a limit of c_mute_groups_max (32) at
     *  mute-group setup time.
     */

    mutegroup::number m_group_selected;

    /**
     *  If true, indicates that non-zero mute-groups were present in this MIDI
     *  file.  We need to know if valid mute-groups are present when deciding
     *  whether or not to write them to the 'rc' file.  This can be set by
     *  getting the result of the any() function.
     */

    bool m_group_present;

    /**
     *  Indicates if non-empty mute-groups get saved to the mutes file, MIDI
     *  file, or both.
     */

    saving m_group_save;

    /**
     *  Indicates if non-empty mute-groups get loaded from the mutes file,
     *  MIDI file, or attempted load from both.
     */

    loading m_group_load;

    /**
     *  If true (the default is false), then, when turning off mutes via
     *  toggling, turn off only the patterns that are part of the mute group,
     *  leaving alone any other patterns that the user may have turned on.
     */

    bool m_toggle_active_only;

    /**
     *  If true, and there are no non-zero mutes, then they are not written to
     *  the MIDI file.  The whole "c_mutegroups" SeqSpec section is not
     *  written.
     */

    bool m_strip_empty;

    /**
     *  Indicates the old Seq24/32/64/66 mute-group format, where all values
     *  were stored as longs.
     */

    bool m_legacy_mutes;

public:

    mutegroups
    (
        int rows    = c_rows,
        int columns = c_columns
    );
    mutegroups
    (
        const std::string & name,
        int rows    = c_rows,
        int columns = c_columns
    );

    /*
     * The move and copy constructors, the move and copy assignment operators,
     * and the destructors are all compiler generated.
     */

    mutegroups (const mutegroups &) = default;
    mutegroups & operator = (const mutegroups &) = default;
    mutegroups (mutegroups &&) = default;
    mutegroups & operator = (mutegroups &&) = default;
    ~mutegroups () = default;

    static int Rows ()
    {
        return c_rows;
    }

    static int Columns ()
    {
        return c_columns;
    }

    static int Size ()
    {
        return c_rows * c_columns;
    }

    static bool Swap ()
    {
        return s_swap_coordinates;
    }

    static int null_mute_group ()
    {
        return c_null_mute_group;
    }

    static mutegroup::number grid_to_group (int row, int column);
    static bool group_to_grid(mutegroup::number g, int & row, int & column);
    static saving string_to_group_save (const std::string & value);

    const std::string & name () const
    {
        return m_container_name;
    }

    void name (const std::string & nm)
    {
        if (! nm.empty())
            m_container_name = nm;
    }

    int rows () const
    {
        return m_rows;
    }

    void rows (int r)
    {
        m_rows = r;
    }

    bool group_format_hex () const
    {
        return m_group_format_hex;
    }

    int columns () const
    {
        return m_columns;
    }

    void columns (int c)
    {
        m_columns = c;
    }

    int count () const
    {
        return int(m_container.size());
    }

    bool empty () const
    {
        return m_container.empty();
    }

    int group_count () const
    {
        return m_rows * m_columns;
    }

    bool apply (mutegroup::number group, midibooleans & bits);
    bool unapply (mutegroup::number group, midibooleans & bits);
    bool toggle (mutegroup::number group, midibooleans & bits);
    bool toggle_active (mutegroup::number group, midibooleans & armedbits);

    bool loaded_from_mutes () const
    {
        return m_loaded_from_mutes;
    }

    void group_format_hex (bool flag)
    {
        m_group_format_hex = flag;
    }

    void loaded_from_mutes (bool flag)
    {
        m_loaded_from_mutes = flag;
    }

    saving group_save () const
    {
        return m_group_save;
    }

    bool group_save (const std::string & v);
    bool group_save (saving mgh);
    bool group_save (bool midi, bool mutes);
    std::string group_save_label () const;

    bool group_save_to_mutes () const
    {
        return
        (
            m_group_save == saving::mutes || m_group_save == saving::both
        );
    }

    bool group_save_to_midi () const
    {
        return
        (
            m_group_save == saving::midi || m_group_save == saving::both
        );
    }

    bool saveable_to_midi () const
    {
        return group_save_to_midi() && any();
    }

    loading group_load () const
    {
        return m_group_load;
    }

    bool group_load (const std::string & v);
    bool group_load (loading mgh);
    bool group_load (bool midi, bool mutes);
    std::string group_load_label () const;
    bool load_mute_groups (bool midi, bool mutes);

    bool group_load_from_mutes () const
    {
        return
        (
            m_group_load == loading::mutes || m_group_load == loading::both
        );
    }

    bool group_load_from_midi () const
    {
        return
        (
            m_group_load == loading::midi || m_group_load == loading::both
        );
    }

    /*
     * These functions are useful for setting and retrieving individual mute
     * values.  Our convention is that load() is an interface to a configuration
     * file while set() is an interface for updating existing mute values.
     * add_defaults() is for use by mutegroupsfile.
     */

    bool reset_defaults ();                         /* used in mutegroupsfile */
    bool load (mutegroup::number gmute, const midibooleans & bits);
    bool set (mutegroup::number gmute, const midibooleans & bits);
    midibooleans get (mutegroup::number gmute) const;
    midibooleans get_active_groups () const;
    bool any () const;
    bool any (mutegroup::number gmute) const;
    const mutegroup & mute_group (mutegroup::number gmute) const;
    mutegroup & mute_group (mutegroup::number gmute);
    void show
    (
        const std::string & tag,
        mutegroup::number gmute = c_null_mute_group
    ) const;

    int armed_count (mutegroup::number gmute) const
    {
        return mute_group(gmute).armed_count();
    }

    int group_names_letter_count () const;

    const std::string & group_name (mutegroup::number gmute) const
    {
        return mute_group(gmute).name();
    }

    void group_name (mutegroup::number gmute, const std::string & n)
    {
        mute_group(gmute).name(n);
    }

    bool clear ();

    container & list ()
    {
        return m_container;
    }

    const container & list () const
    {
        return m_container;
    }

    bool group_event () const
    {
        return m_group_event;
    }

    bool group_error () const
    {
        bool result = m_group_error;
        m_group_error = false;
        return result;
    }

    bool group_mode () const
    {
        return m_group_mode;
    }

    bool is_group_learn () const
    {
        return m_group_learn;
    }

    mutegroup::number group_selected () const
    {
        return m_group_selected;
    }

    bool group_valid () const
    {
        return group_valid(group_selected());
    }

    bool group_valid (int g) const
    {
        return g >= 0 && g < c_mute_groups_max;
    }

    bool group_present () const
    {
        return m_group_present;
    }

    void set_group_present ()
    {
        m_group_present = any();
    }

    /**
     *  Provides common code to keep the group value valid even in variset mode.
     *
     * \param group
     *      The group value to be checked and rectified as necessary.
     *
     * \return
     *      Returns the group parameter, clamped between 0 and the number of
     *      mutes (mutegroup object).
     */

    mutegroup::number clamp_group (mutegroup::number group) const
    {
        if (group < 0)
            return 0;
        else if (group >= count())
            return count() - 1;

        return group;
    }

    bool check_group (mutegroup::number group) const
    {
        return (group >= 0) && (group < count());
    }

    bool toggle_active_only () const
    {
        return m_toggle_active_only;
    }

    bool legacy_mutes () const
    {
        return m_legacy_mutes;
    }

    bool strip_empty () const
    {
        return m_strip_empty;
    }

public:         // setters that need to be public

    bool update (mutegroup::number gmute, const midibooleans & bits);
    void group_learn (bool flag);

    void group_event (bool flag)
    {
        m_group_event = flag;
    }

    void group_error (bool flag)
    {
        m_group_error = flag;
    }

    void legacy_mutes (bool flag)
    {
        m_legacy_mutes = flag;
    }

    void toggle_active_only (bool flag)
    {
        m_toggle_active_only = flag;
    }

    void toggle_group_mode ()
    {
        m_group_mode = ! m_group_mode;
    }

    void strip_empty (bool flag)
    {
        m_strip_empty = flag;
    }

    void group_selected (mutegroup::number mg)
    {
        if (group_valid(mg) || mg == c_null_mute_group)
            m_group_selected = mg;
    }

    void group_mode (bool flag)
    {
        m_group_mode = flag;
    }

private:

    void create_empty_mutes ();
    bool add (mutegroup::number gmute, const mutegroup & m);

};              // class mutegroups

}               // namespace seq66

#endif          // SEQ66_MUTEGROUPS_HPP

/*
 * mutegroups.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

