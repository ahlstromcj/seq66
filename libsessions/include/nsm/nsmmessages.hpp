#if ! defined SEQ66_NSMMESSAGES_HPP
#define SEQ66_NSMMESSAGES_HPP

/**
 * \file          nsmmessages.hpp
 *
 *    This module provides macros for generating simple messages, MIDI
 *    parameters, and more.
 *
 * \library       seq66
 * \author        Chris Ahlstrom and other authors; see documentation
 * \date          2020-03-07
 * \updates       2020-03-07
 * \version       $Revision$
 * \license       GNU GPL v2 or above
 *
 *  Provides encapsulated access to the complete set of NSM messages.
 */

/*
 *  Do not document a namespace; it breaks Doxygen.
 */

namespace seq66
{

extern const char * nsm_basic_error ();
extern const char * nsm_basic_list ();
extern const char * nsm_basic_reply ();
extern const char * nsm_cli_gui_hidden ();
extern const char * nsm_cli_gui_shown ();
extern const char * nsm_cli_hide_opt_gui ();
extern const char * nsm_cli_is_clean ();
extern const char * nsm_cli_is_dirty ();
extern const char * nsm_cli_is_loaded ();
extern const char * nsm_cli_label ();
extern const char * nsm_cli_message ();
extern const char * nsm_cli_open ();
extern const char * nsm_cli_progress ();
extern const char * nsm_cli_save ();
extern const char * nsm_cli_show_opt_gui ();
extern const char * nsm_gui_announce ();
extern const char * nsm_gui_remove ();
extern const char * nsm_gui_resume ();
extern const char * nsm_gui_save ();
extern const char * nsm_gui_stop ();
extern const char * nsm_proxy_label ();
extern const char * nsm_proxy_save_signal ();
extern const char * nsm_proxy_stop_signal ();
extern const char * nsm_proxy_kill ();
extern const char * nsm_proxy_start ();
extern const char * nsm_proxy_update ();
extern const char * nsm_srv_abort ();
extern const char * nsm_srv_announce ();
extern const char * nsm_srv_broadcast ();
extern const char * nsm_srv_close ();
extern const char * nsm_srv_list ();
extern const char * nsm_srv_new ();
extern const char * nsm_srv_quit ();
extern const char * nsm_default_ext ();

extern const char * nsm_dirty_msg (bool isdirty);
extern const char * nsm_visible_msg (bool isvisible);
extern const char * nsm_url ();
extern bool nsm_is_announce (const char * s = "");

}           // namespace seq66

#endif      // SEQ66_NSMMESSAGES_HPP

/*
 * nsmmessages.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */

