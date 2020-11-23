#if ! defined SEQ66_MUTEGROUPS_HPP
#define SEQ66_MUTEGROUPS_HPP

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
 * \file          mutegroups.hpp
 *
 *  This module declares a container for a number of optional mutegroup objects.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-12-01
 * \updates       2020-11-23
 * \license       GNU GPLv2 or above
 *
 *  This module is meant to support the main mute groups and the mute groups
 *  from the 'mutes' file.
 */

#include <map>                          /* std::map<> for mutegroup storage */

#include "cfg/basesettings.hpp"         /* seq66::basesettings class        */
#include "play/mutegroup.hpp"           /* seq66::mutegroup stanza class    */

/**
 *  We force a maximum number of mute-groups.  We really only have enough keys
 *  available for 32 mute-groups.
 */

#define SEQ66_MUTE_GROUPS_MAX           32

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
    friend class midifile;
    friend class performer;
    friend class mutegroupsfile;
    friend class setmapper;

public:

    /**
     *  Provides settings for muting.
     */

    enum class muting
    {
        toggle  = -1,
        off     = 0,
        on      = 1
    };

    /**
     *  Provides mutually-exclusive codes for handling the reading of
     *  mute-groups from the "rc" file versus the "MIDI" file.  There's no GUI
     *  way to set this item yet.
     *
     *  handling::mutes: In this option, the mute groups are
     *  writtin only to the "mutes" (formerly "rc") file.
     *
     *  handling::midi: In this option, the mute groups are
     *  only written to the "rc" file if the MIDI file did not contain
     *  non-zero mute groups.  This option prevents the contamination of the
     *  "rc" mute-groups by the MIDI file's mute-groups.  We're going to make
     *  this the default option.  NEEDS FIXING!
     *
     *  handling::both: This is the legacy (seq66) option, which
     *  reads the mute-groups from the MIDI file, and saves them back to the
     *  "rc" file and to the MIDI file.  However, for Seq66 MIDI files such as
     *  b4uacuse-stress.midi, seq66 never reads the mute-groups in that MIDI
     *  file!  In any case, this can be considered a corruption of the "rc"
     *  file.
     */

    enum class handling
    {
        mutes,              /**< Save mute groups to the "mutes" file.      */
        midi,               /**< Write mute groups only to the MIDI file.   */
        both,               /**< Write the mute groups to both files.       */
        maximum             /**< Keep this last... it is only a size value. */
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
     *  Holds a set of mutegroup objects keyed by the configured set number.
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
     *  Indicates if the control values were loaded from an "rc" configuration
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
     *  the c_midi_control_mod_gmute handler or when the group-off or group-on
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
     *  the value should not be used.
     */

    mutegroup::number m_group_selected;

    /**
     *  If true, indicates that non-zero mute-groups were present in this MIDI
     *  file.  We need to know if valid mute-groups are present when deciding
     *  whether or not to write them to the "rc" file.  This can be set by
     *  getting the result of the any() function.
     */

    bool m_group_present;                   // m_mute_group_present

    /**
     *  Indicates if empty mute-groups get saved to the MIDI file.
     */

    handling m_group_save;

public:

    mutegroups
    (
        int rows = SEQ66_DEFAULT_SET_ROWS,
        int columns = SEQ66_DEFAULT_SET_COLUMNS
    );
    mutegroups
    (
        const std::string & name,
        int rows = SEQ66_DEFAULT_SET_ROWS,
        int columns = SEQ66_DEFAULT_SET_COLUMNS
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

    int group_size () const
    {
        return m_rows * m_columns;
    }

    mutegroup::number calculate_mute (int row, int column) const;

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

    handling group_save () const
    {
        return m_group_save;
    }

    bool group_save (const std::string & v);
    bool group_save (handling mgh);
    bool group_save (bool midi, bool mutes);

    std::string group_save_label () const;

    /**
     * \getter m_mute_group_save
     * \return
     *      Returns true if mute-group-handling is set to mutes or both.
     */

    bool group_save_to_mutes () const
    {
        return
        (
            m_group_save == handling::mutes || m_group_save == handling::both
        );
    }

    /**
     * \getter m_mute_group_save
     * \return
     *      Returns true if mute-group-handling is set to midi or both.
     */

    bool group_save_to_midi () const
    {
        return
        (
            m_group_save == handling::midi || m_group_save == handling::both
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
    midibooleans get (mutegroup::number gmute);
    bool any () const;
    const mutegroup & mute_group (mutegroup::number gmute) const;
    void show (mutegroup::number gmute = (-1)) const;

    int armed_count (mutegroup::number gmute) const
    {
        return mute_group(gmute).armed_count();
    }

    const std::string & group_name (mutegroup::number gmute) const
    {
        return mute_group(gmute).name();
    }

    bool clear ()
    {
        bool result = const_cast<mutegroups *>(this)->any();
        m_container.clear();
        return result;
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

private:

    bool add (mutegroup::number gmute, const mutegroup & m);

    container & list ()
    {
        return m_container;
    }

    void group_event (bool flag)
    {
        m_group_event = flag;
    }

    void group_error (bool flag)
    {
        m_group_error = flag;
    }

    void group_mode (bool flag)
    {
        m_group_mode = flag;
    }

    void toggle_group_mode ()           // was toggle_mode_group_mute ()
    {
        m_group_mode = ! m_group_mode;
    }

    void group_learn (bool flag)
    {
        if (flag)
            m_group_mode = m_group_learn = true;
        else
            m_group_learn = false;
    }

    void group_selected (mutegroup::number mg)
    {
        m_group_selected = mg;                  /* need to validate     */
    }

};              // class mutegroups

}               // namespace seq66

#endif          // SEQ66_MUTEGROUPS_HPP

/*
 * mutegroups.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

