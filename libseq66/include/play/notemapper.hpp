#ifndef SEQ66_NOTEMAPPER_HPP
#define SEQ66_NOTEMAPPER_HPP

/*
 * midicvtpp - A MIDI-text-MIDI translater
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/**
 * \file          notemapper.hpp
 *
 *    This module provides functions for advanced MIDI/text conversions.
 *
 * \library       libmidipp
 * \author        Chris Ahlstrom
 * \date          2014-04-24
 * \updates       2019-11-05
 * \version       $Revision$
 * \license       GNU GPL
 *
 *    The mapping process works though static functions that reference a
 *    global notemapper object.
 *
 *    This object gets its setup from an INI file.  This INI file has an
 *    unnamed section with the following format:
 *
\verbatim
         gm-channel = 10
         device-channel = 16
\endverbatim
 *
 *    The "drum" sections are named for the GM note that is to be
 *    remapped.
 *
\verbatim
         [ Drum 35 ]
         gm-name  = Acoustic Bass Drum
         gm-note  = 35
         dev-note = 35
\endverbatim
 *
 */

#include <map>
#include <string>

#include "cfg/basesettings.hpp"         /* seq66::basesettings class        */

namespace seq66
{

 /**
  * This class provides for some basic remappings to be done to MIDI
  * files, using the old and new facilities of libmidifilex.
  *
  *    It works by holding all sorts of standard C++ map objects that are
  *    used to translate from one numeric value to another.
  *
  *    For use in the midicvtpp application, a single global instance of
  *    this object is created, and is used in static C-style callback
  *    functions that can be used in the C library libmidifilex.
  */

class notemapper final : public basesettings
{

    friend void show_maps
    (
        const std::string & tag,
        const notemapper & container,
        bool full_output
    );

public:

    /**
     *  Provides a constant to indicate an inactive or invalid integer
     *  value.
     */

    static const int NOT_ACTIVE = -1;

private:

    /**
     *  This class is meant to extend the map of values with additional data
     *  that can be written out to summarize some information about the MIDI
     *  remapping that was done.  Instead of just the integer value to use,
     *  this class holds the names of the items on both ends of the mapping,
     *  plus a usage count.  We also added the "GM equivalent" name to this
     *  class as well.
     */

    class notepair
    {

    private:

        /**
         *  The incoming note number from a non-GM compliant device.  This
         *  value is used as a "key" value in a map.
         */

        const int m_dev_value;

        /**
         *    The integer value to which the incoming (key) value is to be
         *    mapped.  This is the value of the drum note on a GM-compliant
         *    device.
         */

        const int m_gm_value;

        /**
         *    The name of the GM drum note or patch that is replacing the
         *    device's drum note of patch.  Sometimes there is no exact
         *    replacement, so it is good to know what GM sound is replacing the
         *    device's sound.
         */

        const std::string m_gm_name;

        /**
         *    The number of times this particular mapping was performed in the
         *    MIDI remapping operation.
         */

        int m_remap_count;

     public:

        notepair () = delete;
        notepair (int devvalue, int gmvalue, const std::string & gmname);
        notepair (const notepair &) = default;
        notepair & operator = (const notepair &) = default;

        int dev_value () const
        {
           return m_dev_value;
        }

        int gm_value () const
        {
           return m_gm_value;
        }

        const std::string & gm_name () const
        {
           return m_gm_name;
        }

        void increment_count ()
        {
           ++m_remap_count;
        }

        int count () const
        {
           return m_remap_count;
        }

     };       // class notepair

 private:

    /**
     *    Provides the type of the map between one set of values and
     *    another set of values.
     */

    using midimap = std::map<int, notepair>;
//  using iterator = midimap::iterator;
//  using const_iterator = midimap::const_iterator;
//  using midimap_pair = std::pair<int, notepair>;
//  using midimap_result = std::pair<iterator, bool>;
//  using intmap_result = std::pair<std::map<int, int>::iterator, bool>;

 private:

    /**
     *    Indicates what kind of mapping is allegedly provided by the file.
     *    This can be one of the following values:
     *
     *       -  "drums".  The file describes mapping one pitch/channel to
     *          another pitch/channel, used mostly for coercing old drum
     *          machines to something akin to a General MIDI kit.
     *       -  "patches".  The file describes program (patch) mappings, used
     *          to map old devices patch change values to General MIDI.
     *          Not yet supported.
     *       -  "multi".  The file describes both "drums" and "patchs"
     *          mappings.  Not yet supported.
     *
     *    The name of this attribute in the INI file is "map-type".  Case
     *    is significant.
     */

    std::string m_map_type;

    /**
     *    Provides the number of records (lines) or sections in the INI
     *    file.  Indicates the number of items being remapped.
     *
     *    This attribute ("record-count") does not appear in the INI file,
     *    as it is calculated as the file is read.
     *
     * \warning
     *    Only applies to "drum" mappings at present.  MUST FIX!
     */

    int m_record_count;

    /**
     *    Provides the channel to use for General MIDI drums.  This value
     *    is usually 9, meaning MIDI channel 10.  However, be careful, as
     *    externally, this value is always on a 1-16 scale, while
     *    internally it is reduced by 1 (a 0-15 scale) to save endless
     *    decrements.
     *
     *    The name of this attribute in the INI file is "gm-channel".  Case
     *    is significant.
     */

    int m_gm_channel;

    /**
     *    Provides the channel that is used by the native device.  Older
     *    MIDI equipment sometimes used channel 16 for percussion.
     *
     *    The name of this attribute in the INI file is "dev-channel".
     *    Case is significant.
     */

    int m_device_channel;

    /**
     *    Indicates that the mapping should occur in the reverse direction.
     *    That is, instead of mapping the notes from the device pitches and
     *    channel to General MIDI, the notes and channel should be mapped
     *    from General MIDI back to the device.  This option is useful for
     *    playing back General MIDI files on old equipment.
     *
     *    Note that this option is an INI option ("reverse"), as well as a
     *    command-line option.  It is specified by alternate means, such as
     *    a command-line parameter like "--reverse".
     */

    bool m_map_reversed;

    /**
     *    Provides the mapping between pitches.  If m_map_reversed is
     *    false, then the key is the GM pitch/note, and the value is the
     *    device pitch/note (which is the GM note needed to produce the
     *    same sound in GM as the device would have produced).  If
     *    m_map_reversed is true, then the key is the GM pitch/note, and
     *    the value is the device pitch/note, so that the MIDI file will be
     *    converted from GM mapping to device mapping.
     */

    midimap m_note_map;

    /**
     *    Provides the mapping between channels (optional).  If
     *    m_map_reversed is true, then the mapping of channels is reversed.
     *    There's no need for channel names with this one.
     */

    std::map<int, int> m_channel_map;

    /**
     *    Indicates if the setup is valid.
     */

    bool m_is_valid;

 public:

    notemapper ();                      // an unnamed,  no-change mapping
    notemapper                          // arguments are passed to initree
    (
       const std::string & name,
       const std::string & filespec  = "",    // for behavior like C version
       bool reverse_it               = false,
       int filter_channel            = NOT_ACTIVE,
       bool reject_it                = false,
       const std::string & infile    = "",
       const std::string & outfile   = ""
    );

    bool add (int devnote, int gmnote, const std::string & gmname);
    int repitch (int channel, int input);

    /**
     *    Determines if the value parameter is usable, or "active".
     *
     * \param value
     *    The integer value to be checked.
     *
     * \return
     *    Returns true if the value is not NOT_ACTIVE.
     */

    static bool active (int value)
    {
       return value != notemapper::NOT_ACTIVE;
    }

    /**
     *    Determines if both value parameters are usable, or "active".
     *
     * \param v1
     *    The first integer value to be checked.
     *
     * \param v2
     *    The second integer value to be checked.
     *
     * \return
     *    Returns true if both of the values are not NOT_ACTIVE.
     */

    static bool active (int v1, int v2)
    {
       return
       (
           v1 != notemapper::NOT_ACTIVE &&
           v2 != notemapper::NOT_ACTIVE
       );
    }

    const std::string & map_type () const
    {
       return m_map_type;
    }

    int record_count () const
    {
       return m_record_count;
    }

    int gm_channel () const
    {
       return m_gm_channel + 1;
    }

    int device_channel () const
    {
       return m_device_channel + 1;
    }

    bool valid () const
    {
       return m_is_valid;
    }

    const midimap & note_map () const
    {
       return m_note_map;
    }

    bool map_reversed () const
    {
       return m_map_reversed;
    }

public:

    void map_type (const std::string & mp)
    {
        m_map_type = mp;
    }

    void map_reversed (bool flag)
    {
        m_map_reversed = flag;
    }

    void gm_channel (int ch)
    {
       m_gm_channel = ch - 1;
    }

};                  // class notemapper

}                   // namespace seq66

#endif              // SEQ66_NOTEMAPPER_HPP

 /*
  * notemapper.hpp
  *
  * vim: sw=4 ts=4 wm=8 et ft=cpp
  */

