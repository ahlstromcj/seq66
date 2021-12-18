#if ! defined SEQ66_LISTSBASE_HPP
#define SEQ66_LISTSBASE_HPP

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
 * \file          listsbase.hpp
 *
 *  An abstract base class for inputslist and clockslist.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2020-12-11
 * \updates       2021-12-18
 * \license       GNU GPLv2 or above
 *
 *  Defines the list of MIDI inputs and outputs (clocks).  We've combined them
 *  for "convenience". :-) Oh, and for port-mapping.
 */

#include <string>                       /* std::string                      */
#include <map>                          /* std::map<buss, I/O struct>       */

#include "midi/midibus_common.hpp"      /* enum class e_clock, etc.         */
#include "midi/midibytes.hpp"           /* bussbyte and other types         */

#define USE_ALIAS_IF_PRESENT            /* EXPERIMENTAL                     */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  A wrapper for a vector of clocks and inputs values, as used in
 *  mastermidibus and the performer object.
 */

class listsbase
{
    friend std::string output_port_map_list ();
    friend std::string input_port_map_list ();

protected:

    /**
     *  Provides a port name and the input or output values.  Note that the
     *  clock setting will be off (not disabled) for all input values.  This
     *  is so that we can disable missing inputs when port-mapping.  The clock
     *  setting will be disabled for output values that are actually disabled
     *  by the user or are missing from the actual system ports.
     */

    using io = struct
    {
        bool io_enabled;            /**<< The status setting for this buss. */
        e_clock out_clock;          /**<< The clock setting for this buss.  */
        std::string io_name;        /**<< The name of the I/O buss.         */
        std::string io_nick_name;   /**<< The short name of the I/O buss.   */
        std::string io_alias;       /**<< FYI only, and only for JACK.      */
    };

    /**
     *  The container type for io information.  Replaces std::vector<io>.
     */

    using container = std::map<bussbyte, io>;


    /**
     *  Saves the input or clock settings obtained from the "rc" (options)
     *  file so that they can be loaded into the mastermidibus once it is
     *  created.
     */

    container m_master_io;

    /**
     *  Indicates if the list is to be used.  It will always be saved and read,
     *  but not used if this flag is false.
     */

    bool m_is_active;

    /**
     *  Indicates if this list is a port-mapper list.  Useful in debugging.
     */

    bool m_is_port_map;

public:

    listsbase (bool pmflag = false);
    virtual ~listsbase () = default;

    virtual std::string io_list_lines () const = 0;
    virtual bool add_list_line (const std::string & line);

    static bool parse_port_line
    (
        const std::string & line,
        int & portnumber,
        int & portstatus,
        std::string & portname
    );

    void match_up (const listsbase & source);

    void clear ()
    {
        m_master_io.clear();
    }

    void deactivate ()
    {
        clear();
        m_is_active = false;
    }

    int count () const
    {
        return int(m_master_io.size());
    }

    bool not_empty () const
    {
        return ! m_master_io.empty();
    }

    bool active () const
    {
        return m_is_active && not_empty();
    }

    bool is_port_map () const
    {
        return m_is_port_map;
    }

    void active (bool flag)
    {
        m_is_active = flag;
    }

    void set_name (bussbyte bus, const std::string & name);
    void set_nick_name (bussbyte bus, const std::string & name);
    void set_alias (bussbyte bus, const std::string & name);
    std::string get_name (bussbyte bus, bool addnumber = false) const;
    std::string get_nick_name (bussbyte bus, bool addnumber = false) const;
    std::string get_alias (bussbyte bus) const;
    bussbyte bus_from_nick_name (const std::string & nick) const;
    bussbyte bus_from_alias (const std::string & alias) const;
    std::string port_name_from_bus (bussbyte nominalbuss) const;
    void show (const std::string & tag) const;
    bool is_disabled (bussbyte bus) const;

protected:

    container & master_io ()
    {
        return m_master_io;
    }

    const container & master_io () const
    {
        return m_master_io;
    }

    std::string to_string (const std::string & tag) const;
    std::string extract_nickname (const std::string & name) const;
    std::string e_clock_to_string (e_clock e) const;
    std::string port_map_list (bool isclock) const;
    std::string io_line
    (
        int portnumber,
        int status,
        const std::string & portname,
        const std::string & portalias = ""
    ) const;
    bool add
    (
        int buss,
        int status,
        const std::string & name,
        const std::string & nickname = "",
        const std::string & alias = ""
    );
    bool add (int buss, io & ioitem, const std::string & nickname);
    const io & get_io_block (const std::string & nickname) const;

};              // class listsbase

}               // namespace seq66

#endif          // SEQ66_LISTSBASE_HPP

/*
 * listsbase.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

