#if ! defined SEQ66_PATCHES_HPP
#define SEQ66_PATCHES_HPP

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
 * \file          patches.hpp
 *
 *  This module declares the array of MIDI controller names.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2025-02-17
 * \updates       2025-02-18
 * \license       GNU GPLv2 or above
 *
 */

#include <map>                          /* std::map<> template class        */
#include <string>                       /* std::string<> template class     */

#include "midi/midibytes.hpp"           /* seq66::midibyte type             */

namespace seq66
{

/**
 *  Provides a small wrapper class for an alternate mapping of patch numbers
 *  (program numbers) to patch names.
 */

class patches
{
    friend std::string program_name (int patchnumber);

public:

    using container = std::map<int, std::string>;

private:

    /**
     *  A container for the up to 128 pairs of patch numbers and names.
     *  Initially of size zero.
     */

    container m_patch_map;

    /**
     *  Indicates if the patch map is to be used in place of the built-in
     *  GM patch list.
     */

    bool m_active;

    /**
     *  Holds the [comments] for the patch file.
     */

    std::string m_comments;

public:

    patches () = default;               /* an empty, inactive map   */
    patches (const patches &) = delete;
    const patches & operator = (const patches &) = delete;
    ~patches () = default;

    const container & patch_map () const
    {
        return m_patch_map;
    }

    void clear ()
    {
        m_patch_map.clear();
        activate(false);
    }

    bool add (int patchnumber, const std::string & patchname);
    std::string name (int patchnumber) const;

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

    std::string name_ex (int patchnumber) const;

};          // class patches

/*
 *  Acessor functions
 */

extern bool add_patch (int patchnumber, const std::string & patchname);
extern void set_patches_comment (const std::string & c);
extern const std::string & get_patches_comment ();
extern std::string program_name (int patchnumber);
extern std::string program_list ();
extern std::string gm_program_name (int patchnumber);

}           // namespace seq66

#endif      // SEQ66_PATCHES_HPP

/*
 * patches.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

