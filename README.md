# README for Seq66 0.96.0

Chris Ahlstrom
2015-09-10 to 2021-07-10

__Seq66__ is a MIDI sequencer and live-looper with a hardware-sampler
grid-pattern interface, MIDI automation for live performance, sets and
playlists for song management, scale/chord-aware piano-roll interface, song
editor for creative composition, and control via mouse, keystrokes, and MIDI.
Mute-groups can enable/disable multiple patterns with one keystroke or MIDI
control. Supports NSM (Non Session Manager) on Linux; can also be run headless.
It does not support audio samples, just MIDI.

__Seq66__ is a major refactoring of Sequencer64/Kepler34, both being reboots
of __Seq24__ with modern C++ and new features.  Linux users can build this
application from the source code.  See the INSTALL file.  Windows users can
get an installer package on GitHub or build it with Qt Creator.  A comprehensive
PDF user-manual is provided.

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

    *   Version 0.96.0:
        *   Fixed harmonic transposition settings.
        *   Fixed bug that drew fingerprint at top of button; and another bug
            that disabled showing the pattern if fingerprinting was disabled.
        *   Can now read MIDI files that are readable, but not writable, and
            disable File / Save as needed.
        *   Added hjkl arrow keys to seqroll and perfroll. Why? We have a
            Microsoft Arc keyboard with all the arrows on one small button.
        *   Fixed the Mixolydian mode scale. Oops.
        *   The pattern editor time, roll, data, and event frames line up.
        *   More work on pattern-editor in tab. More to come.
    *   Version 0.95.1:
        *   Added 'auto' option to song-start-mode setting.
        *   Added missing harmonic-transposition option to the pattern editor.
        *   Fixed issues with some of the file-dialogs.
        *   Fixed issues with editing keystrokes being passed from the tabbed
            editors to the main window (and the live frame).
        *   Improved arrow-key handling in seqroll.
        *   Removed PortMidi Java configuration parsing code.
        *   Important fixes to NSM sessions.
        *   Added 'one-shot' option the the 'usr' file's record-style setting.
        *   Improved support for making the user-interface smaller; 4x4 sets.
        *   Added the "enigmatic" scale.
        *   Fixes to handling of musical key, scale, and background pattern.
        *   Finalized the scale-finder code (Ctrl-K in the pattern editor's
            piano roll.)
        *   Updated libtool support, now seq66cli is under 200K in size.
        *   Removed QtWidget directory from qclocklayout #includes for issue
            #54.  Also added error-message and about to the Qt m4 file.
    *   Version 0.95.0:
        *   Updated almost all settings to use "name = variable" convention
            to improve readability.  This will change the format of config-files
            slightly upon exit.
        *   Fixes made to the sample Launchpad Mini 'ctrl' files.  Also added a
            "Quit" command, especially useful in a headless run.
        *   Added 8 "Alt" MIDI Control Out values to support special cases
            not covered by the concrete set of control-out values.
        *   Major cleanup and neatening of header files.
        *   Minor bugs and regressions fixed along the way.
        *   Loop-edit key will create a new sequence if slot is empty.
        *   Patterns empty of playable events no longer show unnecessary
            progress bars.
        *   The estimated duration of a tune is shown in the Song tab, based
            on pattern lengths and song triggers.
    *   Version 0.94.0:
        *   Major improvements to the stability of MIDI control of playlists.
        *   Added Phrygian scale thanks to user WinkoErades.
        *   Update the 'rc' file for a simpler specification of JACK MIDI and
            JACK transport.
        *   Removed the old-style live frame; use themes and style-sheets to
            create the old look if you like it.
        *   Removed a lot of debugging material of no use now.
        *   Fixed (mostly) a weird bug causing the application to hang
            on exit when set as JACK Master.  Pretty damn weird.
        *   Improving support for non-default PPQNs.
        *   Patterns can now be copied and merged into a new pattern to combine
            (for example) separate percussion tracks.
        *   Fixed error in playlist::verify() adding an empty list.
        *   Improving the handling of headless seq66cli; added a "quit"
            automation function for MIDI-only use.
        *   Fixed some issues with reading Cakewalk WRK files.

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
