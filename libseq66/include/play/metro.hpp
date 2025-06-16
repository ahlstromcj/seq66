#if ! defined SEQ66_METRO_HPP
#define SEQ66_METRO_HPP

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
 * \file          metro.hpp
 *
 *      Provides a configurable pattern that can be used as a metronome,
 *      plus add addition pattern class that can be used for background
 *      recording.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2022-08-05
 * \updates       2022-08-18
 * \license       GNU GPLv2 or above
 *
 *  The metro is a sequence with a special configuration.  It can be added
 *  to the performer's playset to be played along with the rest of the
 *  patterns.  It is not visible and it is not editable once created.
 *  There is also a lot of stuff in seq66::sequence not needed here.
 *
 *  The recorder class extends the metro class for recording in the background
 *  automatically.
 */

#include "play/sequence.hpp"            /* seq66::sequence                  */

namespace seq66
{

/**
 *  Configuration class for the metro class. It covers the members of the
 *  metro class pluse the bus, channel, beats, and beat width.
 */

class metrosettings
{
private:

    /**
     *  Provides the desired MIDI buss and channel to play the metronome.
     */

    bussbyte m_buss;
    midibyte m_channel;

    /**
     *  Provides the desired MIDI buss to record from when doing
     *  background recording. No channel is forced on the pattern;
     *  the user can apply the desired channel later.
     */

    bussbyte m_recording_buss;

    /**
     *  Provides the desired MIDI buss and channel to send the background
     *  recording events out to be heard.
     */

    bussbyte m_thru_buss;
    midibyte m_thru_channel;

    /**
     *  Provides the desired time-signature of the metronome.
     */

    int m_beats_per_bar;
    int m_beat_width;

    /**
     *  Provides the patch/program number to use.  This selects the
     *  sound the metronome should have.  It is played at the start of each
     *  loop; added first in the event list.
     */

    midibyte m_main_patch;

    /**
     *  Optionally, the other beats can be played with a different patch.
     */

    midibyte m_sub_patch;

    /**
     *  The highlight (measure) note to play, its velocity, and its length.
     *  The length ends up being calculated using the beat width, PPQN, and
     *  the note-fraction members below.
     */

    midibyte m_main_note;
    midibyte m_main_note_velocity;
    midipulse m_main_note_length;

    /**
     *  The sub-measure (beat) notes to play, their velocity, and their
     *  lengths.
     */

    midibyte m_sub_note;
    midibyte m_sub_note_velocity;
    midipulse m_sub_note_length;

    /**
     *  Provides the fraction of beat width used for the length of the main
     *  and sub notes.
     */

    float m_main_note_fraction;
    float m_sub_note_fraction;

    /**
     *  Support for count-in.  This involves a boolean to indicate it is active,
     *  the number of measures to count in, and whether recording (to a hidden
     *  record pattern is activated.
     *
     *  We may need to add a recording buss number to the configuration.
     */

    bool m_count_in_active;
    int m_count_in_measures;

    /**
     *  Additional support for background recording.
     */

    bool m_count_in_recording;
    int m_recording_measures;

public:

    metrosettings ();

    midipulse calculate_length (int increment, float fraction);
    bool initialize (int increment);
    void set_defaults ();

    bool sanity_check () const
    {
        return m_main_note > 0 && m_sub_note > 0;
    }

    bussbyte buss () const
    {
        return m_buss;
    }

    midibyte channel () const
    {
        return m_channel;
    }

    bussbyte recording_buss () const
    {
        return m_recording_buss;
    }

    bussbyte thru_buss () const
    {
        return m_thru_buss;
    }

    midibyte thru_channel () const
    {
        return m_thru_channel;
    }

    int beats_per_bar () const
    {
        return m_beats_per_bar;
    }

    int beat_width () const
    {
        return m_beat_width;
    }

    midibyte main_patch () const
    {
        return m_main_patch;
    }

    midibyte sub_patch () const
    {
        return m_sub_patch;
    }

    midibyte main_note () const
    {
        return m_main_note;
    }

    midibyte main_note_velocity () const
    {
        return m_main_note_velocity;
    }

    float main_note_fraction () const
    {
        return m_main_note_fraction;
    }

    midipulse main_note_length () const
    {
        return m_main_note_length;
    }

    midibyte sub_note () const
    {
        return m_sub_note;
    }

    midibyte sub_note_velocity () const
    {
        return m_sub_note_velocity;
    }

    float sub_note_fraction () const
    {
        return m_sub_note_fraction;
    }

    midipulse sub_note_length () const
    {
        return m_sub_note_length;
    }

    bool count_in_active () const
    {
        return m_count_in_active;
    }

    int count_in_measures () const
    {
        return m_count_in_measures;
    }

    bool count_in_recording () const
    {
        return m_count_in_recording;
    }

    int recording_measures () const
    {
        return m_recording_measures;
    }

    bool expand_recording () const
    {
        return m_recording_measures == 0;
    }

public:

    void buss (int b)
    {
        if (! is_null_buss(b))
            m_buss = bussbyte(b);
    }

    void channel (int ch)
    {
        if (is_good_channel(ch))
            m_channel = midibyte(ch);
    }

    void recording_buss (int b)
    {
        if (! is_null_buss(b))
            m_recording_buss = bussbyte(b);
    }

    void thru_buss (int b)
    {
        if (! is_null_buss(b))
            m_thru_buss = bussbyte(b);
    }

    void thru_channel (int ch)
    {
        if (is_good_channel(ch))
            m_thru_channel = midibyte(ch);
    }

    void beats_per_bar (int bpb)
    {
        m_beats_per_bar = bpb;
    }

    /*
     * Since this is not saved, we don't care if it is not a power of
     * two.
     */

    void beat_width (int bw)
    {
        m_beat_width = bw;
    }

    void main_patch (int patch)
    {
        if (is_good_data_byte(patch))
            m_main_patch = midibyte(patch);
    }

    void sub_patch (int patch)
    {
        if (is_good_data_byte(patch))
            m_sub_patch = midibyte(patch);
    }

    void main_note (int note)
    {
        if (is_good_data_byte(note))
            m_main_note = midibyte(note);
    }

    void main_note_velocity (int vel)
    {
        if (is_good_data_byte(vel))
            m_main_note_velocity = midibyte(vel);
    }

    void main_note_fraction (float fraction)
    {
        if (fraction == 0.0 || (fraction >= 0.125 && fraction <= 2.0))
            m_main_note_fraction = fraction;
    }

    void sub_note (int note)
    {
        if (is_good_data_byte(note))
            m_sub_note = midibyte(note);
    }

    void sub_note_velocity (int vel)
    {
        if (is_good_data_byte(vel))
            m_sub_note_velocity = midibyte(vel);
    }

    void sub_note_fraction (float fraction)
    {
        if (fraction == 0.0 || (fraction >= 0.125 && fraction <= 2.0))
            m_sub_note_fraction = fraction;
    }

    void count_in_active (bool flag)
    {
        m_count_in_active = flag;
    }

    void count_in_measures (int count)
    {
        m_count_in_measures = count;
    }

    void count_in_recording (bool flag)
    {
        m_count_in_recording = flag;
    }

    void recording_measures (int m)
    {
        m_recording_measures = m;
    }

};          // class metrosettings

/**
 *  The metro class is just a sequence used for implementing a metronome
 *  functionality.
 */

class metro : public sequence
{
    friend class performer;

private:

    metrosettings m_metro_settings;

private:

    metro & operator = (const metro & rhs);

public:

    metro ();
    metro (const metrosettings & ms);
    virtual ~metro ();

    virtual bool initialize (performer * p);
    virtual bool uninitialize ()
    {
        return true;
    }

    metrosettings & settings ()
    {
        return m_metro_settings;
    }

protected:

    bool init_setup (performer * p, int measures);

};          // class metro

/**
 *  An extension of metro for recording in the backbround.
 */

class recorder final : public metro
{
    friend class performer;

private:

    recorder & operator = (const recorder & rhs);

public:

    recorder ();
    recorder (const metrosettings & ms);
    virtual ~recorder ();

    virtual bool initialize (performer * p) override;
    virtual bool uninitialize () override;

};          // class recorder

}           // namespace seq66

#endif      // SEQ66_METRO_HPP

/*
 * recorder.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

