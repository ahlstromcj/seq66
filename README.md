# README for Seq66 0.92.0 (Sequencer64 refactored for C++/14 and Qt 5)

Chris Ahlstrom
2015-09-10 to 2021-03-05

__Seq66__ is a MIDI sequencer and live MIDI looper with a hardware sampler-like
grid pattern interface, MIDI automation for live performance, with sets and
playlists for song management, a scale/chord-aware piano-roll interface, a
song editor for creative composition, and control via mouse, keystrokes, and
MIDI.  Supports NSM (New Session Manager) on Linux, can also be run headless.

__Seq66__ is a refactoring of the Qt version of Sequencer64/Kepler34, a reboot
of __Seq24__ with modern C++ and new features.  Linux users can build this
application from the source code.  See the INSTALL file.  Windows users can
get an installer package on GitHub.  A PDF user-manual is also provided.

# Major Features (also see **Recent Changes** below):

##  User interface

    *   Qt 5 (good cross-platform support).  No "grid of sets", but
        unlimited external windows.
    *   Tabs for management of sets, mute-groups, song mode, pattern
        editing, event-editing, play-lists, and session information.
    *   The live frame uses buttons matching Qt theming.
    *   A color for each sequence can be chosen to make them stand out.
        The color number is saved in a *SeqSpec* associated with the track.
        The color palette can be saved and modified.

##  Configuration files

    *   Separated MIDI control and mute-group setting into their own files,
        with support for hex notation.
    *   Supported configuration files: ".rc", ".usr", ".ctrl", ".mutes",
        ".playlist", ".drums" (note-mapping), and ".palette".
    *   Unified keystroke control and MIDI control into a ".ctrl" file. It
        defines MIDI In controls for controlling Seq66, and MIDI Out controls
        for displaying Seq66 status in grid controllers (e.g.  LaunchPad).
        Basic 4x8 and 8x8 ".ctrl" files for the Launchpad Mini provided.

##  Non Session Manager

    *   Support for this manager is essentially complete.
    *   Handles stopping and saving
    *   Handle display of details about the session.

##  Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce the need for polling.

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

    *   Version 0.92.1:
        *   Fixed issue #42 by adding scrollbars to I/O lists in Preferences
            when there are many ports on the system; also increased port
            limits to 48 ports.
        *   Fixed issues with iterating through MIDI events in the
            user-interfaces.
        *   Big fixes to Song recording, still in progress.
        *   Added an experimental one-shot auto-step step-edit feature for
            recording (for example) drumbeats from MIDI devices.
        *   Fixed bug in parsing Fluidsynth port names.
        *   Improved handling of MIDI I/O control of Seq66.  Added more
            automation-display events, more testing.
        *   Updated documentation.
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

/*
 * vim: sw=4 ts=4 wm=4 et ft=markdown
 */
