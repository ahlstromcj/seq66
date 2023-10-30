#if ! defined SEQ66_QSEQEVENTFRAME_HPP
#define SEQ66_QSEQEVENTFRAME_HPP

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
 *  Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qseqeventframe.hpp
 *
 *  This module declares/defines the edit frame for sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-08-13
 * \updates       2023-05-05
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>                       /* used as a base class             */
#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */

#include "play/performer.hpp"           /* seq66::performer::callbacks base */
#include "play/seq.hpp"                 /* seq66::seq::pointer & sequence   */
#include "util/basic_macros.hpp"        /* nullptr and related macros       */
#include "qseventslots.hpp"             /* seq66::qseventslots              */

/**
 *  Forward reference.
 */

class QTableWidgetItem;

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace Ui
{
    class qseqeventframe;
}

/*
 * Do not document namespaces, it breaks Doxygen.
 */

namespace seq66
{
    class qseventslots;

class qseqeventframe final :
    public QFrame,
    protected performer::callbacks
{
    friend class qseventslots;

private:

    /**
     *  Provides human-readable names for the columns of the event table.
     */

    enum class column_id
    {
        timestamp,
        eventname,
        channel,
        data_0,
        data_1,
        link
    };

    Q_OBJECT

public:

    qseqeventframe
    (
        performer & p,
        sequence & s,
        QWidget * parent = nullptr
    );
    virtual ~qseqeventframe ();

    void set_initialized ()
    {
         m_initialized = true;
    };

protected:

    virtual bool on_sequence_change
    (
        seq::number seqno, performer::change ctype  // bool recreate
    ) override;

    /*
     * virtual bool on_group_learn (bool state) override;
     * virtual bool on_set_change (screenset::number setno) override;
     */

private:

    void set_selection_multi (bool multi);
    void set_row_heights (int height);
    void set_row_height (int row, int height);
    void set_column_widths (int total_width);
    void set_seq_title (const std::string & title);
    void set_seq_time_sig_and_ppqn (const std::string & sig);
    void set_seq_lengths (const std::string & mevents);
    void set_seq_channel (const std::string & channel);
    void set_event_category (const std::string & c);
    void set_event_timestamp (const std::string & ts);
    void set_event_name (const std::string & n);
    void set_event_channel (int channel);
    void set_event_data_0 (const std::string & d);
    void set_event_data_1 (const std::string & d);
    void set_event_plaintext (const std::string & t);
    void set_event_system (const std::string & t);
    void set_event_seqspec (const std::string & t);
    void set_event_line
    (
        int row,
        const std::string & evtimestamp,
        const std::string & evname,
        const std::string & evchannel,
        const std::string & evdata0,
        const std::string & evdata1,
        const std::string & linktime
    );
    void set_event_line (int row, const editable_event & ev);   /* overload */
    void set_event_line (int row);                              /* overload */
    void set_dirty (bool flag = true);
    bool initialize_table ();
    std::string make_seq_title ();
    std::string get_lengths ();

    sequence & track ()
    {
        return m_seq;
    }

private:

    QTableWidgetItem * cell (int row, column_id col);
    void current_row (int row);
    int current_row () const;
    void populate_category_combo ();
    void populate_status_combo ();
    void populate_system_combo ();
    void populate_meta_combo ();
    void populate_seqspec_combo ();
    void populate_midich_combo ();
    std::string filename_prompt
    (
        const std::string & prompt,
        const std::string & file
    );

protected:                          // overrides of event handlers

    virtual void keyPressEvent (QKeyEvent *) override;
    virtual void keyReleaseEvent (QKeyEvent *) override;

private slots:

    void slot_table_click_ex (int row, int column, int prevrow, int prevcol);
    void slot_row_selected ();
    void slot_link_status ();
    void slot_delete ();
    void slot_insert ();
    void slot_modify ();
    void slot_save ();
    void slot_clear ();
    void slot_dump ();
    void slot_cancel ();
    void update_seq_name ();
    void slot_midi_channel (int index);
    void slot_event_name (int index);
    void slot_event_category (int index);
    void slot_hex_data_state (int state);
    void slot_pulse_time_state (int state);
    void slot_meta_text_change ();

private:

    Ui::qseqeventframe * ui;

private:

    /**
     *  Provides a reference to the sequence that this dialog is meant to view
     *  or modify.
     */

    sequence & m_seq;

    /**
     *  This object holds an editable_events container, and helps this
     *  user-interface class manage the list of events.
     */

    std::unique_ptr<qseventslots> m_eventslots;

    /**
     *  If true, selecting a note event also selects its linked event.
     */

    bool m_linked_selection;

    /**
     *  If true, show the data bytes in hexadecimal format.
     */

    bool m_show_data_as_hex;

    /**
     *  If true, show the time as pulses instead of B:B:T.
     */

    bool m_show_time_as_pulses;

    /**
     *  Indicates the dialog has now been set up.  We need this to
     *  avoid dirtying the dialog during setup. Weird.
     */

    bool m_initialized;

    /**
     *  Indicates a modification is active.
     */

    bool m_is_dirty;

    /**
     *  Holds the index of the channel named "None".
     */

    int m_no_channel_index;

};          // class qseqeventframe

}           // namespace seq66

#endif      // QSEQEVENTFRAME_HPP

/*
 * qseqeventframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

