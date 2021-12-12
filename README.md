# README for Seq66 0.98.1

Chris Ahlstrom
2015-09-10 to 2021-12-12

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

    *   Version 0.98.1:
        *   Work on creating an NSIS installer for a 64-bit Windows build.
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
    *   Version 0.97.3:
        *   Added pattern-recording indicator to live-grid slots.
        *   Replace useless "Record" automation command with a "Loop Mode"
            command to move between normal loop mode in the Live grid to various
            record-toggling modes:  Overdub (merge), Overwrite, Expand, and
            One-shot.
        *   Also refactored the quantize control to move through states of
        *   normal, quantize, and tighten.
        *   Refactored MIDI control for possible future usage of the D1 event
            value.
        *   Fixes to external live grid handling.
        *   Fixed the display of loop status changed via MIDI control.
        *   Fixed and updated the Windows build.  Mitigated high CPU usage
            when not (!) playing; fixed a bug in microsleep() for Windows.

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
