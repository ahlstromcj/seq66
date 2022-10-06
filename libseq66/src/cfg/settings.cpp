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
 * \file          settings.cpp
 *
 *  This module declares/defines just some of the global (gasp!) variables
 *  and functions in this application.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2016-05-17
 * \updates       2022-10-04
 * \license       GNU GPLv2 or above
 *
 *  The first part of this file defines a couple of global structure
 *  instances, followed by the global variables that these structures
 *  completely replace.  The second part includes some convenience functions.
 */

#include <stdexcept>                    /* std::invalid_argument            */

#include "cfg/settings.hpp"             /* std::rc(), usr(), and much more  */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

/**
 *  The combolist default constructor.  Currrently used in qseditoptions and
 *  qsmainwnd.
 */

combolist::combolist (bool use_current) :
    m_list_items    (),
    m_use_current   (use_current)
{
    if (m_use_current)
        m_list_items.push_back("");
}

combolist::combolist (const tokenization & slist, bool use_current) :
    m_list_items    (),
    m_use_current   (use_current)
{
    if (m_use_current)
        m_list_items.push_back("");

    for (const auto & s : slist)
        m_list_items.push_back(s);
}

void
combolist::current (const std::string & s) const
{
    if (m_use_current)
    {
        combolist * ncthis = const_cast<combolist *>(this);
        ncthis->m_list_items[0] = s;
    }
}

void
combolist::current (int v) const
{
    if (m_use_current)
    {
        combolist * ncthis = const_cast<combolist *>(this);
        ncthis->m_list_items[0] = std::to_string(v);            // FIXME !
    }
}

std::string
combolist::at (int index) const
{
    std::string result;
    if (index >= 0 && index < int(m_list_items.size()))
        result = m_list_items[index];

    return result;
}

int
combolist::ctoi (int index) const
{
    int result = (-1);
    std::string s = at(index);
    if (! s.empty())
    {
        try
        {
            result = std::stoi(s);
        }
        catch (std::invalid_argument const &)
        {
            // no code
        }
    }
    return result;
}

int
combolist::index (const std::string & target) const
{
    int result = 0;                         /* for failure, use first item  */
    int counter = 0;
    for (const auto & s : m_list_items)
    {
        if (counter == 0 && m_use_current)
        {
            ++counter;
            continue;
        }
        if (s == target)
        {
            result = counter;
            break;
        }
        ++counter;
    }
    return result;
}

int
combolist::index (int value) const
{
    std::string target = std::to_string(value);
    return index(target);
}

/**
 *  These lists are useful in the user-interfaces.
 */

const tokenization &
measure_items ()
{
    static const tokenization s_measure_list
    {
        "1", "2", "3", "4", "5", "6", "7", "8",
        "16", "24", "32", "64", "96", "128"
    };
    return s_measure_list;
}

/**
 *  Beats-per-bar values.
 */

const tokenization &
beats_per_bar_items ()
{
    static const tokenization s_beats_per_bar_list
    {
        "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "10", "11", "12", "13", "14", "15", "16", "32"
    };
    return s_beats_per_bar_list;
}

/**
 *  MIDI cannot store any beat-width values that are not powers of 2.
 *  So we do not show them.  The user can still enter such values, but
 *  problems will ensue.
 */

const tokenization &
beatwidth_items ()
{
    static const tokenization s_beatwidth_list
    {
        "1", "2", "4", "8", "16", "32"
    };
    return s_beatwidth_list;
}

/**
 *  These static items are used to fill in and select the proper snap values for
 *  the grids.  These values are also used for note length.  See
 *  update_grid_snap() and update_note_length() in qseqeditframe.
 */

const tokenization &
snap_items ()
{
    static const tokenization s_snap_list
    {
        "1", "2", "4", "8", "16", "32", "64", "128", "-",
        "3", "6", "12", "24", "48", "96", "192"
    };
    return s_snap_list;
}

/**
 *  Snap values for the performance/song editor when snap is enabled.
 *  These values are converted to 1/1, 1/2, etc. and represent fractions
 *  of a meansure.  With snap disabled, the full length of the pattern is
 *  applicable, and trigger sliding is arbitrary.
 */

const tokenization &
perf_snap_items ()
{
    static const tokenization s_snap_list
    {
        "1", "2", "3", "4", "8", "16", "32"
    };
    return s_snap_list;
}

/**
 *  Zoom values for the pattern editor.
 */

const tokenization &
zoom_items ()
{
    static const tokenization s_zoom_list
    {
        "1", "2", "4", "8", "16", "32", "64", "128", "256", "512"
    };
    return s_zoom_list;
}

/**
 *  Recording-volume values for the pattern editor.  Holds the entries for the
 *  "Vel" drop-down.  The first value matches usr().preserve_velocity().  It
 *  corresponds to the "Free" recording-volume entry where the incoming
 *  velocity is kept.
 */

const tokenization &
rec_vol_items ()
{
    static const tokenization s_rec_vol_list
    {
        "Free", "127", "112", "96", "80", "64", "48", "32", "16"
    };
    return s_rec_vol_list;
}

/**
 *  Returns a reference to the global rcsettings object.  Why a function
 *  instead of direct variable access?  Encapsulation.  We are then free to
 *  change the way "global" settings are accessed, without changing client
 *  code.
 *
 * \return
 *      Returns the global object g_rcsettings.
 */

rcsettings &
rc ()
{
    static rcsettings s_rcsettings;
    return s_rcsettings;
}

/**
 *  Provides the replacement for all of the other settings in the "user"
 *  configuration file, plus some of the "constants" in the globals module.
 *  Returns a reference to the global usrsettings object, for better
 *  encapsulation.
 *
 * \return
 *      Returns the global object g_usrsettings.
 */

usrsettings &
usr ()
{
    static usrsettings g_usrsettings;
    return g_usrsettings;
}

/**
 *  Call set_defaults() on the "rc" and "usr" objects.
 */

void
set_configuration_defaults ()
{
    rc().set_defaults();
    usr().set_defaults();
}

#if defined USE_PPQN_LIST_VALUE

/**
 *  Available PPQN values.  The default is 192.  The first item uses the edit
 *  text for a non-standard default PPQN (like 333).  However, note that the
 *  default PPQN can be edited by the user to be any value within this range.
 *  Also note this list is wrapped in an accessor function.
 *
 * \param index
 *      Provides the index into the PPQN list.  If set to below 0, then
 *      it represents a request for the size of the list.
 *
 * \return
 *      Returns either the desired PPQN value, or the length of the list.
 *      If the index is not in the range, and is not (-1), then 0 is returned,
 *      and the result should be ignored.
 */

int
ppqn_list_value (int index)
{
    static int s_ppqn_list [] =
    {
        0,                          /* place-holder for default PPQN    */
        32, 48, 96, 192, 240,
        384, 768, 960, 1920, 2400,
        3840, 7680, 9600, 19200
    };
    static const int s_count = sizeof(s_ppqn_list) / sizeof(int);
    int result = 0;
    s_ppqn_list[0] = usr().default_ppqn();
    if (index >= 0 && (index < s_count))
        result = s_ppqn_list[index];
    else if (index == (-1))
        result = s_count;

    return result;
}

#endif  // defined USE_PPQN_LIST_VALUE

/**
 *  This list is useful in the user-interface.  Also see ppqn_list_value()
 *  below for internal integer versions. Used in qsmainwnd and in
 *  qseditoptions.
 */

const tokenization &
default_ppqns ()
{
    static tokenization s_default_ppqn_list
    {
        "32", "48", "96", "192", "240",
        "384", "768", "960", "1920", "2400",
        "3840", "7680", "9600", "19200"
    };
    return s_default_ppqn_list;
}

/**
 *  Used for overriding the JACK server's buffer size.  The first entry
 *  translates to 0, and disables this override.
 */

const tokenization &
jack_buffer_size_list ()
{
    static tokenization s_buffer_size_list
    {
        "0",                              /* do not override the JACK server  */
        "16",                             /* might not work on many systems   */
        "32",                             /* might not work on some systems   */
        "64",                             /* might not work on a few systems  */
        "128", "256", "512",
        "1024", "2048", "4096",
        "8192"
    };
    return s_buffer_size_list;
}

/**
 *  Common code for handling PPQN settings.  Putting it here means we can
 *  reduce the reliance on the global PPQN, and have a lot more flexibility in
 *  changing the PPQN.
 *
 *  However, this function works completely only if the "user" configuration
 *  file has already been read.  In some cases we may need to retrofit the
 *  desired PPQN value!
 *
 * \param ppqn
 *      Provides the PPQN value to be used. The default value is
 *      c_use_default_ppqn.
 *
 * \return
 *      Returns the ppqn parameter, unless that parameter is one of the
 *      special values above, or is illegal, as noted above.
 */

int
choose_ppqn (int ppqn)
{
    int result = ppqn;
    if (result == c_use_default_ppqn)
        result = usr().midi_ppqn();                 /* usr().default_ppqn() */
    else if (result == c_use_file_ppqn)
        result = usr().file_ppqn();
    else if (! ppqn_in_range(result))               /* file, in-range PPQN  */
        result = usr().midi_ppqn();                 /* usr().default_ppqn() */

    return result;
}

/**
 *  Indicates if the PPQN value is in the legal range of usable PPQN values.
 */

bool
ppqn_in_range (int ppqn)
{
    return usr().use_file_ppqn() || usr().is_ppqn_valid(ppqn);
}

/**
 *  Returns the file-name of the Seq66 user's manual.
 */

const std::string &
pdf_user_manual ()
{
    static const std::string name = "seq66-user-manual.pdf";
    return name;
}

/**
 *  Returns the path to the github version of the Seq66 user's manual.
 */

const std::string &
pdf_user_manual_url ()
{
    static const std::string url =
        "https://ahlstromcj.github.io/docs/seq66/seq66-user-manual.pdf";

    return url;
}

/**
 *  This list is useful to look up the installed documentation.
 */

const tokenization &
doc_folder_list ()
{
#if defined SEQ66_PLATFORM_WINDOWS
    static tokenization s_folder_list
    {
        "C:/Program Files (x86)/Seq66/data/doc",
        "C:/Program Files/Seq66/data/doc",
        ""                              /* terminator   */
    };
#else
    static bool s_uninitialized = true;
    static tokenization s_folder_list;
    static std::string s_usr_dir;
    static std::string s_usr_local_dir;
    if (s_uninitialized)
    {
        s_usr_dir = "/usr/share/doc/" + seq_api_subdirectory();
        s_usr_local_dir = "/usr/local/share/doc/" + seq_api_subdirectory();
        s_folder_list.push_back(s_usr_dir);
        s_folder_list.push_back(s_usr_local_dir);
        s_folder_list.push_back("data/share/doc");  /* source distro file?  */
#if defined SEQ66_PLATFORM_DEBUG
        s_folder_list.push_back("../seq66/data/share/doc");  /* shadow dir  */
#endif
        s_uninitialized = false;
    }
#endif
    return s_folder_list;
}

/**
 *  Currently, we don't store the tutorial on the web in
 *  ahlstromcj.github.io.
 */

const tokenization &
tutorial_folder_list ()
{
#if defined SEQ66_PLATFORM_WINDOWS
    static tokenization s_folder_list
    {
        "C:/Program Files (x86)/Seq66/data/doc/tutorial",
        "C:/Program Files/Seq66/data/doc/tutorial",
        ""                              /* terminator   */
    };
#else
    static bool s_uninitialized = true;
    static tokenization s_folder_list;
    static std::string s_usr_dir;
    static std::string s_usr_local_dir;
    if (s_uninitialized)
    {
        s_usr_dir = "/usr/share/doc/" + seq_api_subdirectory();
        s_usr_local_dir = "/usr/local/share/doc/" + seq_api_subdirectory();
        s_usr_dir += "/tutorial";
        s_usr_local_dir += "/tutorial";
        s_folder_list.push_back(s_usr_dir);
        s_folder_list.push_back(s_usr_local_dir);
        s_folder_list.push_back("data/share/doc/tutorial");
#if defined SEQ66_PLATFORM_DEBUG
        s_folder_list.push_back("../seq66/data/share/doc/tutorial");
#endif
        s_uninitialized = false;
    }
#endif
    return s_folder_list;
}

}           // namespace seq66

/*
 * settings.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

