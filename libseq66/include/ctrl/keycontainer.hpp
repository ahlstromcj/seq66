#if ! defined SEQ66_KEYCONTAINER_HPP
#define SEQ66_KEYCONTAINER_HPP

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
 * \file          keycontainer.hpp
 *
 *  This module declares/defines the class for holding MIDI operation data for
 *  the application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-11-18
 * \updates       2021-04-24
 * \license       GNU GPLv2 or above
 *
 *  This container holds a map of keycontrol objects keyed by a key ordinal
 *  number that can range from 0 to 255.
 *
 *  It requires C++11 and above.
 */

#include <map>                          /* std::map<>                       */
#include <string>                       /* std::string                      */

#include "ctrl/keycontrol.hpp"          /* seq66::keycontrol                */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  Provides an object specifying what a keystroke, GUI action, or a MIDI
 *  control should do.
 */

class keycontainer
{

    friend class midicontrolin;

public:

    /**
     *  Provides the type definition for this container.  The key is the
     *  operation number (generally ranging from 0 to 31 for each
     *  automation::category), and the value is a midioperation object.
     */

    using keymap = std::map<ctrlkey, keycontrol>;

    /**
     *  An additional container to hold the pattern-offset numbers and the
     *  corresponding keystrokes to quickly look up the keystroke name based on
     *  the offset number.
     */

    using slotmap = std::map<int, std::string>;

    /**
     *  An additional container to hold the mute-offset numbers and the
     *  corresponding keystrokes to quickly look up the keystroke name based on
     *  the offset number.
     */

    using mutemap = std::map<int, std::string>;

    /**
     *  Helps to work around key-board control issues.
     */

    enum class layout
    {
        qwerty,
        qwertz,
        azerty,
        max
    };

private:

    /**
     *  Defines a small structure type for holding some default values,
     *  using in initializing the key-container when there is no configuration
     *  file.
     */

    using keydefault = struct
    {
        std::string kd_name;            /**< The human-readable key name.   */
        automation::action kd_action;   /**< The action for that key.       */
    };

    /**
     *  The container itself.
     */

    keymap m_container;

    /**
     *  A name to use for showing the contents of the container.
     */

    std::string m_container_name;

    /**
     *  Reverse lookup map for pattern-offset numbers.
     */

    slotmap m_pattern_keys;

    /**
     *  Reverse lookup map for mute-offset numbers.
     */

    mutemap m_mute_keys;

    /**
     *  Indicates if the key values were loaded from an "rc" configuration
     *  file, as opposed to using the default values of the keys.
     */

    bool m_loaded_from_rc;

    /**
     *  Indicates if the auto-shift feature for group learning is to be used.
     *  Defaults to true, but some keyboard layouts (e.g. AZERTY) are
     *  problematic with this feature.
     */

    bool m_use_auto_shift;

    /**
     *  Defaults to layout::qwerty.
     */

    layout m_kbd_layout;

public:

    keycontainer ();
    keycontainer (const std::string & name);

    const std::string & name () const
    {
        return m_container_name;
    }

    void clear ()
    {
        m_container.clear();
        m_pattern_keys.clear();
        m_mute_keys.clear();
    }

    int count () const
    {
        return int(m_container.size());
    }

    bool add (ctrlkey ordinal, const keycontrol & kc);
    bool add_slot (const keycontrol & kc);
    bool add_mute (const keycontrol & kc);
    const keycontrol & control (ctrlkey ordinal) const;
    std::string slot_key (int pattern_offset) const;
    std::string mute_key (int mute_offset) const;

    bool loaded_from_rc () const
    {
        return m_loaded_from_rc;
    }

    void loaded_from_rc (bool flag)
    {
        m_loaded_from_rc = flag;
    }

    bool use_auto_shift () const
    {
        return m_use_auto_shift;
    }

    void use_auto_shift (bool flag)
    {
        m_use_auto_shift = flag;
    }

    layout kbd_layout () const
    {
        return m_kbd_layout;
    }

    void kbd_layout (layout lay)
    {
        m_kbd_layout = lay;
    }

    void set_kbd_layout (const std::string & lay);
    std::string kbd_layout_to_string (layout lay);

    std::string kbd_layout_to_string ()
    {
        return kbd_layout_to_string(m_kbd_layout);
    }

public:

    void show () const;

private:

    void add_defaults ();               /* many statics inside this one!    */

    const keymap & container () const
    {
        return m_container;
    }

};              // class keycontainer

}               // namespace seq66

#endif          // SEQ66_KEYCONTAINER_HPP

/*
 * keycontainer.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

