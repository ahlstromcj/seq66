# README for Seq66 0.92.3

Chris Ahlstrom
2015-09-10 to 2021-04-03

__Seq66__ is a MIDI sequencer/live-looper with a hardware-sampler-like
grid-pattern interface, MIDI automation for live performance, sets and
playlists for song management, scale/chord-aware piano-roll interface, song
editor for creative composition, and control via mouse, keystrokes, and MIDI.
Supports NSM (New Session Manager) on Linux, can also be run headless.
Note that it does not support audio, just MIDI.

__Seq66__ is a refactoring of the Qt version of Sequencer64/Kepler34, a reboot
of __Seq24__ with modern C++ and new features.  Linux users can build this
application from the source code.  See the INSTALL file.  Windows users can
get an installer package on GitHub or build it with Qt Creator.  A PDF
user-manual is also provided.

# Major Features

    Also see **Recent Changes** below.

##  User interface

    *   Qt 5 (good cross-platform support).  A grid of loop buttons and a song
        layout tab.  Unlimited external windows.  The live frame uses buttons
        matching Qt theming.  Qt style-sheet support.
    *   Tabs for management of sets, mute-groups, song mode, pattern
        editing, event-editing, play-lists, and session information.
    *   A color for each sequence can be chosen to make them stand out.
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
    *   Handles stopping and saving>
    *   Handles display of details about the session.

##  Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce the need for polling.
    *   A ton of clean-up.

##  Multiple Builds

    *   ALSA/JACK: `qseq66`
    *   Command-line/daemon: `seq66cli`
    *   PortMidi: `qpseq66`
    *   Windows: `qpseq66.exe`

Seq66 uses a Qt 5 user-interface based on Kepler34 and the Seq66 *rtmidi*
(Linux) and *portmidi* (Windows) engines.  MIDI devices are detected,
inaccessible devices are ignored, with playback (e.g. to the Windows wavetable
synth). It is built easily via *GNU Autotools*, *Qt Creator* or *qmake*, using
*MingW*.  See the INSTALL file for build-from-source instructions for Linux or
Windows, and using a conventional source tarball.

## Recent Changes

    *   Version 0.92.3:
        *   Add a transpose value to song editor triggers to support shifting
            patterns automatically during Song play.
        *   Fixed playback of ultra-long patterns due to not sleeping in the
            output loop (forgot to convert milliseconds to microseconds).
        *   Fixed vertical zoom in the pattern editor.
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
    *   Version 0.92.1:
        *   Fixed issue #42 by adding scrollbars to I/O lists in Preferences
            when there are many ports on the system; also increased port
            limits to 48 ports.
        *   Fixed issues with iterating through MIDI events in the
            user-interfaces.
        *   Big fixes to Song recording. More to do?
        *   Added an experimental one-shot auto-step step-edit feature for
            recording (for example) drumbeats from MIDI devices.
        *   Fixed bug in parsing Fluidsynth port names.
        *   Improved handling of MIDI I/O control of Seq66.  Added more
            automation-display events, more testing.
        *   Updated documentation, including a ODF spreadsheet for the MIDI
            control configuration of the *Launchpad Mini*.
    *   Version 0.92.0:
        *   Fixed issue #34: "seq66 does not follow jack_transport tempo changes"
        *   Fixed issues with applying 'usr' buss and instrument names to the
            pattern-editor menus.
        *   Fixing serious issues with the event editor. Now deletes both
            linked notes.
        *   Added mute-group-out to show mutegroups on Launchpad Mini buttons.
        *   Tightened up mute-group handling, configuration file handling,
            play-list handling, and MIDI display device handling.
        *   Stream-line the format of the 'ctrl' file by removing columns for
            the values of "enabled" and "channel".  Will detect older formats
            and fix them.
        *   A PDF user manual is now provided in the doc directory, installed
            to /usr/local/share/doc/seq66-0.92 with "make install".

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
