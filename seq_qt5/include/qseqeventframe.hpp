#if ! defined SEQ66_QSEQEVENTFRAME_HPP
#define SEQ66_QSEQEVENTFRAME_HPP

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
 *  along with seq66; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

/**
 * \file          qseqeventframe.hpp
 *
 *  This module declares/defines the edit frame for sequences.
 *
 * \library       seq66 application
 * \author        Chris Ahlstrom
 * \date          2018-08-13
 * \updates       2019-09-15
 * \license       GNU GPLv2 or above
 *
 */

#include <QFrame>

#include <memory>                       /* std::shared_ptr<>, unique_ptr<>  */

#include "play/performer.hpp"           /* seq66::performer::callbacks      */
#include "play/seq.hpp"                 /* seq66::seq::pointer & sequence   */
#include "util/basic_macros.hpp"        /* nullptr and related macros       */

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

class qseqeventframe final : public QFrame, protected performer::callbacks
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
        data_1
    };

    Q_OBJECT

public:

    qseqeventframe
    (
        performer & p,
        int seqid,
        QWidget * parent = nullptr
    );

    virtual ~qseqeventframe ();

protected:

    virtual bool on_sequence_change (seq::number seqno) override;

    /*
     * virtual bool on_group_learn (bool state) override;
     * virtual bool on_set_change (screenset::number setno) override;
     */

private:

    void set_row_heights (int height);
    void set_row_height (int row, int height);
    void set_column_widths (int total_width);
    void set_seq_title (const std::string & title);
    void set_seq_time_sig_and_ppqn (const std::string & sig);
    void set_seq_lengths (const std::string & mevents);
    void set_seq_channel (const std::string & channel);

    std::string make_seq_title ();

    void set_event_category (const std::string & c);
    void set_event_timestamp (const std::string & ts);
    void set_event_name (const std::string & n);
    void set_event_data_0 (const std::string & d);
    void set_event_data_1 (const std::string & d);
    void set_event_line
    (
        int row,
        const std::string & evtimestamp,
        const std::string & evname,
        const std::string & evchannel,
        const std::string & evdata0,
        const std::string & evdata1
    );
    void set_dirty (bool flag = true);

    bool initialize_table ();

    std::string get_lengths ();

private:

    QTableWidgetItem * cell (int row, column_id col);
    void set_current_row (int row);

private slots:

    void handle_table_click (int row, int column);
    void handle_table_click_ex (int row, int column, int prevrow, int prevcol);
    void handle_delete ();
    void handle_insert ();
    void handle_modify ();
    void handle_save ();
    void handle_cancel ();
    void update_seq_name ();

private:

    Ui::qseqeventframe * ui;

private:

    /**
     *  Provides a reference to the sequence that this dialog is meant to view
     *  or modify.
     */

    seq::pointer m_seq;

    /**
     *  This object holds an editable_events container, and helps this
     *  user-interface class manage the list of events.  We might be able to
     *  simplify it.
     */

    std::unique_ptr<qseventslots> m_eventslots;

    /**
     *  Indicates the current row.
     */

    int m_current_row;

    /**
     *  Indicates a modification is active.
     */

    bool m_is_dirty;

};          // class qseqeventframe

}           // namespace seq66

#endif      // QSEQEVENTFRAME_HPP

/*
 * qseqeventframe.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

