# README for Seq66 0.97.0

Chris Ahlstrom
2015-09-10 to 2021-09-26

__Seq66__ is a MIDI sequencer and live-looper with a hardware-sampler
grid-pattern interface, MIDI automation for live performance, sets and playlists
for song management, scale/chord-aware piano-roll interface, song editor for
creative composition, and control via mouse, keystrokes, and MIDI.
Mute-groups can enable/disable multiple patterns with one keystroke or MIDI
control. Supports NSM (Non Session Manager) on Linux; can also be run
headless.  It does not support audio samples, just MIDI.

__Seq66__ is a major refactoring of Sequencer64/Kepler34, both being reboots of
__Seq24__ with modern C++ and new features.  Linux users can build this
application from the source code.  See the INSTALL file; it has notes on many
types on installation, including notes for OpenSUSE Tumbleweed.  Windows users can
get an installer package on GitHub or build it with Qt Creator.  A comprehensive
PDF user-manual is provided.

![Alt text](doc/latex/images/main-window/main-window-fluxbox.png?raw=true "Seq66
Dark-Cold Fluxbox")

# Major Features

##  User interface

    *   Qt 5 (cross-platform support).  A grid of loop buttons, unlimited
        external windows. Buttons match a Qt theme.
    *   Qt style-sheet support, to further alter the app's appearance.
    *   Tabs for management of sets, mute-groups, song mode, pattern
        editing, event-editing, play-lists, and session information.
    *   Each pattern slot can be colored; the color palette can be saved and
        modified.
    *   A headless version can be built.

##  Configuration files

    *   Separates MIDI control and mute-group setting into their own files;
        some support for hex notation.
    *   Supports configuration files: ".rc", ".usr", ".ctrl", ".mutes",
        ".playlist", ".drums" (note-mapping), ".palette", and Qt ".qss".
    *   Unified keystroke and MIDI control in the ".ctrl" file; defines MIDI
        In controls for automating Seq66, and MIDI Out controls for displaying
        Seq66 status in grid controllers (e.g. LaunchPad).  Basic 4x8 and 8x8
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
        Works with Song Export.
    *   Improved non-U.S. keyboard support.
    *   Many demonstration and test MIDI files.
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
            Still has vertical scroll issues.
        *   Added more Preferences settings, enabled some that were not yet
            implemented.  Upgraded the handling of the configuration files.
            By default, the 'rc' file is always saved, in case ports change.
        *   Added option "wrap-around" (for notes) to the 'usr' file.
        *   Added option "lock-main-window" to the 'usr' file to prevent
            resizing the window.
        *   Added code to show unlinked note events on the seqroll, the "u"
            key to remove them, and the "=" key to relink them.
        *   Added more build information to --version and Help / Build Info.
        *   Fixed drawing slot-button borders, drawn with no pattern color now.
        *   Got auto-scaling of the slot button font working.
        *   Implemented the Help / About links as per issue #21.
        *   Added OpenSUSE INSTALL notes from the sivecj/Seq66 fork.
        *   Improved the display and editing of tempo events, especially in the
            pattern editor.
        *   Can now copy/paste a pattern from one MIDI file to another.
    *   Version 0.96.3:
        *   Added ability to modify Note On and Note Off at the same time in the
            event editor. Fixed and updated event::get_rank().
        *   Fixed output port-map issue with lookup of "FLUID Synth". Removed
            the random ID number; search the port name via containment, not
            equality.
        *   Usage of the JACK Session API restored, though "deprecated".
            Removed all traces of LASH support. Added a configuration tab
            for allowing session management via JACK or NSM.
        *   Added an automation control for "save session".

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
