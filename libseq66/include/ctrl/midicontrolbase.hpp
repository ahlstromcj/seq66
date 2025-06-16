#ifndef SEQ66_MIDICONTROLBASE_HPP
#define SEQ66_MIDICONTROLBASE_HPP

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
 * \file          midicontrolbase.hpp
 *
 *  This module declares/defines the base class for handling MIDI control
 *  I/O of the application.
 *
 * \library       seq66 application
 * \author        C. Ahlstrom
 * \date          2019-11-25
 * \updates       2022-08-25
 * \license       GNU GPLv2 or above
 *
 *  Provides the base class for midicontrolout.
 *
 * Warning:
 *
 *      It is NOT a base class for midicontrol or midicontrolin!
 */

#include "midi/midibytes.hpp"           /* seq66::bussbyte data type        */

namespace seq66
{

/**
 *  Provides some management support for MIDI control... on I/O.  Many thanks
 *  to igorangst!
 */

class midicontrolbase
{

    friend class qseditoptions;
    friend class midicontrolfile;
    friend class performer;

private:

    /**
     *  A name to use for showing the contents of the container.
     */

    std::string m_name;

    /**
     *  Provides the MIDI I/O buss, that is the port number for MIDI I/O.
     *  This value defaults to 0, and the user must be sure to avoid using
     *  this buss value for music, or redefine the buss.  This is the nominal
     *  buss, which is read and saved, but not used for I/O; see m_true_buss
     *  instead.
     */

    bussbyte m_buss;

    /**
     * The true buss, which exists on the system.
     */

    bussbyte m_true_buss;

    /**
     *  Holds the original value read in from the 'ctrl' file.
     *  It can be modified by an edit in Edit / Preferences, but
     *  actually using it may need to be belayed (e.g. until application
     *  exit completes.
     */

    bussbyte m_configured_buss;

    /**
     *  Indicates that this container is "empty".
     */

    bool m_is_blank;

    /**
     *  Indicates that this container is enabled or disabled.
     */

    bool m_is_enabled;

    /**
     *  Holds the original value read in from the 'ctrl' file.
     *  It can be modified by an edit in Edit / Preferences, but
     *  actually using it may need to be belayed (e.g. until application
     *  exit completes.
     */

    bool m_configure_enabled;

    /**
     *  Offset provides a way to utilize a different portion of a controller
     *  such as the Launchpad Mini.  Currently just set to 0 while we work
     *  things out.
     */

    int m_offset;

    /**
     *  Provides the number of rows, useful when the runtime number of rows
     *  differs from that specified in the configuration file.  We at least
     *  want to avoid segfaults.
     */

    int m_rows;

    /**
     *  Provides the number of rows, useful when the runtime number of rows
     *  differs from that specified in the configuration file.  We at least
     *  want to avoid segfaults.
     */

    int m_columns;

public:

    midicontrolbase (const std::string & name = "");
    virtual ~midicontrolbase () = default;
    virtual bool initialize (int buss, int rows, int columns);      /* base */

    const std::string & name () const
    {
        return m_name;
    }

    bussbyte nominal_buss () const
    {
        return m_buss;
    }

    bussbyte true_buss () const
    {
        return m_true_buss;
    }

    bussbyte configured_buss () const
    {
        return m_configured_buss;
    }

    bool is_blank () const
    {
        return m_is_blank;
    }

    bool is_enabled () const
    {
        return m_is_enabled;
    }

    bool is_disabled () const
    {
        return ! is_enabled();
    }

    bool configure_enabled () const
    {
        return m_configure_enabled;
    }

    int offset () const
    {
        return m_offset;
    }

    int rows () const
    {
        return m_rows;
    }

    int columns () const
    {
        return m_columns;
    }

protected:

    void nominal_buss (bussbyte b)
    {
        m_buss = b;
    }

    void true_buss (bussbyte b)
    {
        if (is_good_buss(b))
            m_true_buss = b;
        else
            is_enabled(false);
    }

    void configured_buss (bussbyte b)
    {
        m_configured_buss = b;
    }

    void is_blank (bool flag)
    {
        m_is_blank = flag;
    }

    void is_enabled (bool flag)
    {
        m_is_enabled = flag;
    }

    void configure_enabled (bool flag)
    {
        m_configure_enabled = flag;
    }

    void offset (int o)
    {
        if (o >= 0)                 /* more verification later  */
            m_offset = o;
    }

    void rows (int r)
    {
        if (r > 0)                  /* more verification later  */
            m_rows = r;
    }

    void columns (int c)
    {
        if (c > 0)                  /* more verification later  */
            m_columns = c;
    }

};          // class midicontrolbase

/*
 *  Free functions.
 */

/**
 *  Default MIDI control input buss.  This value preserves the old behavior,
 *  where the incoming MIDI events of a device on any buss would be acted on
 *  (if specified in the MIDI control stanzas).  This value is the same as
 *  c_bussbyte_max in the midibytes.hpp module.  It can be changed in the
 *  'ctrl' file.
 */

inline bussbyte
default_control_in_buss ()
{
    return null_buss();
}

/**
 *  Default MIDI control output buss.  It is used with igorangst's
 *  MIDI-control-out feature at present.  It can be changed in the 'ctrl'
 *  file.
 */

inline bussbyte
default_control_out_buss ()
{
    return null_buss();
}

}           // namespace seq66

#endif      // SEQ66_MIDICONTROLBASE_HPP

/*
 * midicontrolbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

