# README for Seq66 0.93.1

Chris Ahlstrom
2015-09-10 to 2021-05-03

__Seq66__ is a MIDI sequencer and live-looper with a hardware-sampler
grid-pattern interface, MIDI automation for live performance, sets and
playlists for song management, scale/chord-aware piano-roll interface, song
editor for creative composition, and control via mouse, keystrokes, and MIDI.
Includes mute-groups so that a set of patterns can be enabled/disabled with one
keystroke or MIDI control Supports NSM (New Session Manager) on Linux, can also be
run headless.  It does not support audio samples, just MIDI.

__Seq66__ is a refactoring of the Qt version of Sequencer64/Kepler34, reboots
of __Seq24__ with modern C++ and new features.  Linux users can build this
application from the source code.  See the INSTALL file.  Windows users can
get an installer package on GitHub or build it with Qt Creator.  A large PDF
user-manual is provided.

# Major Features

    Also see **Recent Changes** below.

##  User interface

    *   Qt 5 (good cross-platform support).  A grid of loop buttons and a song
        layout tab.  Unlimited external windows.  The live frame uses buttons
        matching Qt theming.  Qt style-sheet support, to further tinker with the
        app's appearance.
    *   Tabs for management of sets, mute-groups, song mode, pattern
        editing, event-editing, play-lists, and session information.
    *   A color for each pattern can be chosen to make them stand out.
        The color palette can be saved and modified.

##  Configuration files

    *   Separates MIDI control and mute-group setting into their own files,
        with support for hex notation.
    *   Supports configuration files: ".rc", ".usr", ".ctrl", ".mutes",
        ".playlist", ".drums" (note-mapping), and ".palette".
    *   Unified keystroke control and MIDI control in the ".ctrl" file. It
        defines MIDI In controls for controlling Seq66, and MIDI Out controls
        for displaying Seq66 status in grid controllers (e.g.  LaunchPad).
        Basic 4x8 and 8x8 ".ctrl" files for the Launchpad Mini provided.

##  Non Session Manager

    *   Support for this manager is essentially complete.
    *   Handles stopping and saving.
    *   Handles display of details about the session.
    *   Still needs to be integrated with NSM's jackpatch.

##  Multiple Builds

    *   ALSA/JACK: `qseq66`
    *   Command-line/daemon: `seq66cli`
    *   PortMidi: `qpseq66`
    *   Windows: `qpseq66.exe`

##  New Features

    *   Transposable triggers for re-using patterns more comprehensively.
        Works with Song Export. Can be disabled in the 'rc' file.
    *   See **Recent Changes** below.

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

    *   Version 0.93.1:
        *   Work in progress on issue #47; added a keyboard-layout option to the
            'ctrl' file to disable auto-shift and tweak the internal key-map for
            some keyboards.  Added an "AZERTY" 'ctrl' file to the installation.
        *   Improved group-learn key control.
        *   Fixed issue #49, mute-group issues, plus a bug in saving mutes to
            the MIDI file.  Added a couple more flags to the 'mutes' file.
        *   Fixed issue #50, made the slot text color the same as the label
            color, and provided a "secret" default color that will cause the
            text to match the theme.  This can be overridden by a palette file.
        *   Fixed issue #51 (show-stopper!), where playback with JACK transport
            enable was extremely erratic on some platforms.
        *   Add clearing of the performer's "play-set" for "File / New" to
            prevent the previous song from being playable. :-D
        *   Activate usage of (larger) seqedit frame in the "Edit" tab via
            a 'usr' option.  Adapted the size to better fit, but the user still
            needs to increase vertical dimension slightly to see the bottom
            buttons.  Will eventually eliminate the old version of the tabbed
            Edit frame.
        *   Fixed a bug in drawing notes as a new one is recorded; needed to
            "verify-and-link".
        *   Added a loop-count for live playback of a pattern.  0 means normal
            infinite looping; 1 means "one-shot"; and higher numbers will work.
            Stored with the pattern in the MIDI file.
    *   Version 0.93.0:
        *   Added a transpose value to song editor triggers to support shifting
            patterns automatically during Song play.  Added an 'rc' option to
            save MIDI files using the original triggers.
        *   Improved the song editor's vertical scrollbar, it adjusts to the
            number of actual sets.
        *   Added the L/R marker feature to the external pattern editor, and
            Song mode is no longer required to use it.  This makes it easier
            to explore a long pattern for extracting to a short pattern loop.
        *   Added more perfroll snap options, including one to force Seq24
            behavior.
        *   Make the perf-names now also show the pattern color. They highlight
            as the mouse moves over the pattern rows, useful for long songs.
        *   Fixed playback of ultra-long patterns due to not sleeping in the
            output loop (forgot to convert milliseconds to microseconds).
        *   Fixed vertical zoom in the pattern editor.
        *   Fixed a bad bug in writing Meta and SeqSpec data-lengths greater
            than 127.
        *   Enhanced the event editor to work with channel-less tracks.
        *   Added channel and bus menus to the grid-button popup menu.
    *   Version 0.92.2:
        *   Added a Qt "style-sheet" configuration it to the 'usr' file. It
            can be used to alter the appearance of the application beyond what
            a palette can do.  A sample 'qss' file is provided.
        *   Fixed PPQN modification, added user-interface and 'usr'
            configuration to change the default PPQN from 192.
        *   Fixed many issues with changing the time signature.
        *   Fixed creation of new configuration files.
        *   Fixed port-mapping for MIDI output, control, and status display.
        *   Removed the external set-master; use the set-master tab.
        *   Tightened meta-events and set-handling.
        *   More fixes to Song recording; added a Snap button for it.
        *   Fixed the rendering of the beat indicator and pattern fonts.
        *   Updated the man pages and the documentation.

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
