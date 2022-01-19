# README for Seq66 0.98.3

Chris Ahlstrom
2015-09-10 to 2022-01-19

__Seq66__ is a MIDI sequencer and live-looper with a hardware-sampler-like
grid-pattern interface, sets and playlists for song management,
scale/chord-aware piano-roll interface, song editor for creative composition,
and control via MIDI automation for live performance.  Mute-groups
enable/disable multiple patterns with one keystroke or MIDI control. Supports
NSM (Non Session Manager) on Linux; can also run headless.  It does not support
audio samples, just MIDI.

__Seq66__ is a major refactoring of Sequencer64/Kepler34, rebooting __Seq24__
with modern C++ and new features.  Linux users can build this application from
the source code.  See the INSTALL file; it has notes on many types on
installation. Windows users can get an installer package on GitHub or build it
with Qt Creator.  Provides a comprehensive PDF user-manual.

![Alt text](doc/latex/images/main-window/main-windows.png?raw=true "Seq66")

# Major Features

##  User interface

    *   Qt 5 (cross-platform).  A grid of loop buttons, unlimited external
        windows.  Qt style-sheet support.
    *   Tabs for management of sets, mute-groups, song mode, pattern
        editing, event-editing, play-lists, and session information.
    *   Each pattern slot can be colored; the color palette for slots and
        the piano rolls can be saved and modified.
    *   Horizontal and vertical zoom in the pattern and song editors.
    *   A headless version can be built.

##  Configuration files

    *   Separates MIDI control and mute-group setting into their own files;
        some support for hex notation.
    *   Supports configuration files: ".rc", ".usr", ".ctrl", ".mutes",
        ".playlist", ".drums" (note-mapping), ".palette", and Qt ".qss".
    *   Unified keystroke and MIDI control in the ".ctrl" file; defines MIDI
        In controls for automating Seq66, and MIDI Out controls for displaying
        Seq66 status in grid controllers (e.g. LaunchPad).  Basic ".ctrl" files
        are provided for the Launchpad Mini.

##  Non Session Manager

    *   Support for this manager is complete.
    *   Handles stopping and saving.
    *   Handles display of details about the session.
    *   Still needs to be tested with NSM's jackpatch.

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
    *   SMF 0 import and export.
    *   See **Recent Changes** below, and the **NEWS** file.

##  Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce the need for polling.
    *   A ton of clean-up and a lot of refactoring.

Seq66 uses a Qt 5 user-interface based on Kepler34 and the Seq66 *rtmidi*
(Linux) and *portmidi* (Windows) engines.  MIDI devices are detected,
inaccessible devices are ignored, with playback (e.g. to the Windows wavetable
synth). It is built easily via *GNU Autotools*, *Qt Creator* or *qmake*, using
*MingW*.  See the INSTALL file for build-from-source instructions for Linux or
Windows, and using a conventional source tarball.

## Recent Changes

    *   Version 0.98.3:
        *   Fixed issue #76.  Fixed broken MIDI Start handling. Added setting
            the tempo via a tempo event.  Still thinking about MMC.
        *   Fixed old bug in showing note events; they were filtered by the
            pattern's configured channel!
        *   Tweaks to style-sheet handling.
        *   Fixed misuses of msgprintf(). Other minor bug fixes.
        *   In a new NSM session, do not load the most-recent MIDI file, even
            if specified in the imported configuration.  Also, no longer do an
            automatic import of the home configuration to the NSM configuration.
            Instead, use the "File / Import / Import Project" menu entry.
            See the user manual for how this works.
        *   Added a "File / Import / Import Playlist" command.
            See the user manual for how this works.
        *   Made virtual port names in ALSA more consistent.
        *   Added Preferences item for the BPM Precision setting.
        *   Added contrib/code/ametro.c to provide ALSA test for MIDI clocking.
    *   Version 0.98.2:
        *   Fixed issue #74, where -1 for "no buss-override" was being converted
            to 0.
        *   Added detection of missing system ports when mapping ports.
        *   Song duration label is now a button to select time versus measures.
        *   Removed useless flags for loading keystroke and MIDI controls.
        *   Avoid applying mute-group 0 if song has triggers and song-mode is
            'auto'.
        *   Fixes to log-file handling.
        *   Added "Blank" for disabling keystrokes in the 'ctrl' file.
    *   Version 0.98.1:
        *   Work on creating an NSIS installer for a 64-bit Windows build.
        *   Fixed a stupid segfault bug with the --help option.  Doh!
        *   Changed some extended automation keys (for grid modes).
        *   Minor fixes: (1) Set to set 0 when opening a MIDI file; (2) Expand a
            pattern when merging a longer one into it; (3) fixed pattern access
            in sets > 0; (4) fixed an error in string tokenization.
        *   Fixes for port-mapping and naming; if present, the user sets the
            status of the port-map, which is copied to the matching port.
        *   Fixes in song-editor set-names display.
        *   Removed unused module qskeymaps.
    *   Version 0.98.0:
        *   Fixed issue #41 "Hide Seq66 on closing window" via a "visibility"
            automation command and by fixing the response to "hide/show"
            messages from NSM. Note that the NSM API permits the "Quit" command
            to exit the application. Also see the comments at issue #64 "NSM: UI
            show up after restarting the app".
        *   Fixed issue #73 "Compile error because of jack_get_version_string"
            by detecting the presence of this function in configure.ac.
        *   Truly fixed high CPU usage in Windows version of the condition
            variable "wait".  Replaced pthread implementation with C++'s
            condition_variable.
        *   Added "MIDI macros" to the 'ctrl' file.  Can send SysEx or other
            messages from a drop-down list; automatic startup and exit messages.
            More to come.
        *   Added a feature to extract JACK port aliases and show them
            to the user so that the system port can be associated with a named
            device (e.g. "Launchpad-Mini").
        *   Added api_sysex() overrides, at last.
        *   Added Preference items for MIDI control I/O.
        *   Improved the display of the MIDI file-name in title and live grid.
        *   Improving grid-mode functionality, adding more automation controls,
            in progress.

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
