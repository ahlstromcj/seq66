# README for Seq66 0.97.0

Chris Ahlstrom
2015-09-10 to 2021-09-14

__Seq66__ is a MIDI sequencer and live-looper with a hardware-sampler
grid-pattern interface, MIDI automation for live performance, sets and playlists
for song management, scale/chord-aware piano-roll interface, song editor for
creative composition, and control via mouse, keystrokes, and MIDI.
Mute-groups can enable/disable multiple patterns with one keystroke or MIDI
control. Supports NSM (Non Session Manager) on Linux; can also be run
headless.  It does not support audio samples, just MIDI.

__Seq66__ is a major refactoring of Sequencer64/Kepler34, both being reboots of
__Seq24__ with modern C++ and new features.  Linux users can build this
application from the source code.  See the INSTALL file.  Windows users can get
an installer package on GitHub or build it with Qt Creator.  A comprehensive PDF
user-manual is provided.

![Alt text](doc/latex/images/main-window/main-window-fluxbox.png?raw=true "Seq66
Dark-Cold Fluxbox")

# Major Features

##  User interface

    *   Qt 5 (good cross-platform support).  A grid of loop buttons and a song
        layout tab.  Unlimited external windows.  Buttons match Qt theming.
    *   Qt style-sheet support, to further tinker with the app's appearance.
    *   Tabs for management of sets, mute-groups, song mode, pattern
        editing, event-editing, play-lists, and session information.
    *   Each pattern can be colored to stand out.  The color palette can be
        saved and modified.

##  Configuration files

    *   Separates MIDI control and mute-group setting into their own files,
        with support for hex notation.
    *   Supports configuration files: ".rc", ".usr", ".ctrl", ".mutes",
        ".playlist", ".drums" (note-mapping), and ".palette".
    *   Unified keystroke and MIDI control in the ".ctrl" file. It defines MIDI
        In controls for controlling Seq66, and MIDI Out controls for displaying
        Seq66 status in grid controllers (e.g.  LaunchPad).  Basic 4x8 and 8x8
        ".ctrl" files are provided for the Launchpad Mini.

##  Non Session Manager

    *   Support for this manager is essentially complete.
    *   Handles stopping and saving.
    *   Handles display of details about the session.
    *   Still needs to be integrated with NSM's jackpatch.

##  Multiple Builds

    *   ALSA/JACK: `qseq66`
    *   Command-line/headless: `seq66cli`
    *   PortMidi: `qpseq66`
    *   Windows: `qpseq66.exe`

##  More Features

    *   Transposable triggers for re-using patterns more comprehensively.
        Works with Song Export. Can be disabled in the 'rc' file.
    *   Improved non-U.S. keyboard support.
    *   See **Recent Changes** below, and the **NEWS** file.

##  Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce the need for polling.
    *   A ton of clean-up.

Seq66 uses a Qt 5 user-interface based on Kepler34 and the Seq66 *rtmidi*
(Linux) and *portmidi* (Windows) engines.  MIDI devices are detected,
inaccessible devices are ignored, with playback (e.g. to the Windows wavetable
synth). It is built easily via *GNU Autotools*, *Qt Creator* or *qmake*, using
*MingW*.  See the INSTALL file for build-from-source instructions for Linux or
Windows, and using a conventional source tarball.

## Recent Changes

    *   Version 0.97.0:
        *   Added limited vertical zoom to the song editor (performance roll).
            Still has some vertical scrolling issues.
        *   Added more Preferences settings, enabled some that were not yet
            implemented.  Upgraded the handling of the configuration files.
    *   Version 0.96.3:
        *   Added ability to modify Note On and Note Off at the same time in the
            event editor. Fixed and updated event::get_rank().
        *   Fixed output port-map issue with lookup of "FLUID Synth". Removed
            the random ID number; search the port name via containment, not
            equality.
        *   Usage of the JACK Session API restored, though "deprecated".
            Removed all traces of LASH support. Added a configuration tab
            for enabling session management via JACK or NSM.
        *   Added automation control for "save session".
    *   Version 0.96.2:
        *   Fixed issue #55, where the MIDI control channel was being stripped
            when using JACK.  Many other channel-handling improvements.
        *   Fixed a related issue: null (status = 0) control events were sent.
        *   Fixed a bug in "portmidi" MIDI input handling where only the last
            device's events would be processed.
        *   Improved pause/continue support with JACK transport.
        *   Fixed 'rc' allow-click-edit feature.
        *   Made the buss-override settings act more consistently.
        *   Fixed the drawing of preview keys in virtual keyboard.
        *   Fixed setting the modify flag when opening existing pattern.
        *   Added settings interface for the "set mode" of playback.
        *   Added selection of notes and event by channel.
    *   Version 0.96.1:
        *   Restored drag and drop of patterns in the Live frame.
        *   Added the ability to copy all patterns in one set to another set.
        *   Fixed song-start-mode handling when "auto" is specified.
        *   Added "Control" and "Program" (GM) drop-downs in the event editor.
        *   Now display full path names in the recent file list to distinguish
            files better.  An 'rc' config item.
        *   Added a few configuration options to Edit / Preferences.
        *   Added 'usr' options to change the vertical offset of notes in the
            progress-box of each pattern button, and to provide default labeling
            of the piano keys.
        *   Tweaked the drawing of notes and drum notes.
        *   More work on set-handling.
        *   Fixed panic-button disabling Launchpad display output. However,
            if pressed before exit, the lights remain on and there is a slit
            delay in exiting the session (in ALSA; works fine in JACK).
    *   Version 0.96.0:
        *   Adding code to control the visibility of the main window; added
            a "Visibility" MIDI control option, and support for the
            "optional-gui" flag of the Non Session Manager.
        *   Fixed harmonic transposition settings.
        *   Fixed bug that drew fingerprint at top of button; and another bug
            that disabled showing the pattern if fingerprinting was disabled.
        *   Can now read MIDI files that are readable, but not writable, and
            disable File / Save as needed.
        *   Added hjkl arrow keys to seqroll and perfroll. Why? We have a
            Microsoft Arc keyboard with all the arrows on one small button.
        *   Fixed the Mixolydian mode scale. Oops.
        *   The pattern editor time, roll, data, and event frames line up.
        *   Fixed error reading mute-groups; small fixes to configuration files.
        *   Work on fitting pattern-editor in tab.
        *   Work on mitigating weird hang in QApplication destructor.

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
