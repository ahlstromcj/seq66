# README for Seq66 0.98.6

Chris Ahlstrom
2015-09-10 to 2022-04-04

__Seq66__ is a MIDI sequencer and live-looper with a hardware-sampler-like
grid-pattern interface, sets and playlists for song management, a scale and
chord-aware piano-roll interface, song editor for creative composition, and
control via MIDI automation for live performance.  Mute-groups enable/disable
multiple patterns with one keystroke or MIDI control. Supports NSM (Non Session
Manager) on Linux; can also run headless.  It does not support audio samples,
just MIDI.

__Seq66__ is a major refactoring of Sequencer64/Kepler34, rebooting __Seq24__
with modern C++ and new features.  Linux users can build this application from
the source code.  See the INSTALL file; it has notes on many types on
installation.  Windows users can get an installer package on GitHub or build it
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

    *   Support for NSM, New Session Manager, RaySession, Agordejo....
    *   Handles starting, stopping, hiding, and session saving.
    *   Displays details about the session.

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

    *   Version 0.98.6:
        *   Revisited issue #41 to make sure "Quit" is "Hide" under NSM. Also
            fixed issue with the "newtune.midi" default name.
        *   Disabling/enabling JACK input and output on the fly in Preferences
            works.
        *   Added Preferences options to toggle the double-click edit feature
            and select the Live/Song/Auto mode.
        *   Removed long-unused "rtmidi callback" code and other disabled code.
        *   Got user-manual screenshots up-to-date.
        *   Fixed a minor bug involving setting input aliases.
        *   Fixed bug with setting last-used-directory to "".
        *   Upgraded and fixed file-name splitting and rebuilding.
        *   Modify flag now set when painting notes in seqroll.
        *   Fixed handling of Live/Song mode in performer and in 'rc' saving.
        *   Improved appearance of Loop/Record/Quantize buttons in main grid.
        *   Improved modification detection of sequences.
        *   Refactored the reading of global SeqSpecs for better robustness.
        *   Added a file-changed check before restarting the application.
    *   Version 0.98.5:
        *   Added locking for the event-drawing loops to prevent segfaults.
            Active only when recording; prevents iterator invalidation.
        *   Added an underrun indicator to the main window.
        *   The client name ("seq66") is no longer shown, nor saved in the 'rc'
            file. Easier to use multiple Seq66's with the --client-name option.
        *   Updating Dia diagram, finding and fixing issues and dead code.
        *   Added global internal check for both portmaps being active.
        *   Fixed bad slot connection for import_midi_into_session().
        *   Bad command-line options now cause an exit. This includes JACK
            options when Seq66 is built without JACK support.
        *   Improve the coherence of JACK-less builds. Some related NSM tweaks.
        *   Adding JACK access functions for the future. Improved internal
            port initialization code. Removed some non-useful rtmidi code.
        *   Adding the ability to disable/enable input and output on the fly in
            the MIDI Clock and Input tabs.  Still in progress, though.
    *   Version 0.98.4:
        *   Fixed bug in recording-type selector in seqedit. Added a one-shot
            reset option.
        *   Fixed some metadata problems as per issue #75.
        *   Fixed an issue with the H:M:S display being changed by changing the
            beat-width, a bug going back to Seq24's JACK transport support.
        *   MIDI API refactoring for the future; detecting JACK port
            registration.

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
