#if ! defined SEQ66_DRUMS_HPP
#define SEQ66_DRUMS_HPP

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
 * \file          drums.hpp
 *
 *  This module declares the array of MIDI program names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2026-02-16
 * \updates       2026-02-16
 * \license       GNU GPLv2 or above
 *
 */

#include <map>                          /* std::map<> template class        */
#include <string>                       /* std::string<> template class     */

namespace seq66
{

/**
 *  Provides a small wrapper class for an alternate mapping of drum numbers
 *  (program numbers) to drum names.
 */

class drums
{
    friend std::string drum_name (int drumnumber);

public:

    using container = std::map<int, std::string>;

private:

    /**
     *  A container for the up to 128 pairs of drum numbers and names.
     *  Initially of size zero.
     */

    container m_drum_map;

    /**
     *  Indicates if the drum map is to be used in place of the built-in
     *  GM drum list.
     */

    bool m_active;

    /**
     *  Holds the [comments] for the drum file.
     */

    std::string m_comments;

public:

    drums () = default;               /* an empty, inactive map   */
    drums (const drums &) = delete;
    const drums & operator = (const drums &) = delete;
    ~drums () = default;

    const container & drum_map () const
    {
        return m_drum_map;
    }

    void clear ()
    {
        m_drum_map.clear();
        activate(false);
    }

    bool add (int drumnumber, const std::string & drumname);
    std::string name (int drumnumber) const;

    const std::string & comments () const
    {
        return m_comments;
    }

    void comments (const std::string & c)
    {
        m_comments = c;
    }

    bool active () const
    {
        return m_active;
    }

    void activate (bool flag = true)
    {
        m_active = flag;
    }

private:

    std::string name_ex (int drumnumber) const;

};          // class drums

/*
 *  Acessor functions
 */

extern bool add_drum (int drumnumber, const std::string & drumname);
extern void set_drums_comment (const std::string & c);
extern const std::string & get_drums_comment ();
extern std::string drum_name (int drumnumber);
extern std::string program_list ();
extern std::string gm_program_name (int drumnumber);

}           // namespace seq66

#endif      // SEQ66_DRUMS_HPP

/*
 * drums.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
