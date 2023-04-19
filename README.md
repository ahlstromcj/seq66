# README for Seq66 0.99.3 2023-04-20

__Seq66__: MIDI sequencer and live-looper with a hardware-sampler-like
grid-pattern interface, sets, triggers, and playlists for song management,
a scale and chord-aware piano-roll interface, song editor for creative
composition, and control and display via MIDI automation for live performance.
Mute-groups enable/disable multiple patterns with one keystroke/MIDI-control.
Supports NSM (Non/New Session Manager) on Linux; can also run headless.
It does not support audio samples, just MIDI.

__Seq66__ is a major refactoring of Sequencer64/Kepler34/Seq24
with modern C++ and new features.  Linux and Windows users can build this
application from source code.  See the extensive INSTALL file.  Windows users
can get an installer package on GitHub.  A comprehensive PDF user-manual.

Some proposed features will be pushed off to Seq66v2; see the bottom of
the TODO file.

Support sites (still in progress):

    *   https://ahlstromcj.github.io/
    *   https://github.com/ahlstromcj/ahlstromcj.github.io/wiki

![Alt text](doc/latex/images/main-window/main-windows.png?raw=true "Seq66")

# Major Features

##  User interface

    *   Qt 5 (cross-platform).  A grid of loop buttons, unlimited external
        windows.  Qt style-sheet support.
    *   Tabs for management of sets, mute-groups, song mode, pattern
        editing, event-editing, play-lists, and session information.
    *   Low-frequency oscillator (LFO) for modifying continuous controller
        (CC) and velocity values.
    *   An editor for expansion/compression/alignment of note patterns.
    *   Color-able pattern slots; the color palette for slots etc. can be
        saved and modified.
    *   Horizontal and vertical zoom in the pattern and song editors.
    *   A headless version can be built.

##  Configuration files

    *   Separates MIDI control and mute-group setting into their own files.
    *   Supports configuration files: '.rc', '.usr', '.ctrl', '.mutes',
        '.playlist', '.drums' (note-mapping), '.palette', and Qt '.qss'.
    *   Unified keystroke and MIDI controls in the '.ctrl' file; defines MIDI
        controls for automation and displaying Seq66 status in grid
        controllers (e.g. LaunchPad).  Sample '.ctrl' files provided for the
        Launchpad Mini.

##  Non/New Session Manager

    *   Support for NSM/New Session Manager, RaySession, Agordejo....
    *   Handles starting, stopping, hiding, and session saving.
    *   Displays details about the session.

##  Multiple Builds

    *   ALSA/JACK: `qseq66`
    *   Command-line/headless: `seq66cli`
    *   PortMidi: `qpseq66`
    *   Windows: `qpseq66.exe`

##  More Features

    *   Transposable triggers to re-use patterns more comprehensively.
    *   Song export to stock MIDI.
    *   Improved non-U.S. keyboard support.
    *   Many demonstration and test MIDI files.
    *   SMF 0 import and export.
    *   See **Recent Changes** below, and the **NEWS** file.

##  Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce the need for polling.
    *   A ton of clean-up and refactoring.

Seq66 uses a Qt 5 user-interface based on Kepler34 and the Seq66 *rtmidi*
(Linux) and *portmidi* (Windows) engines.  MIDI devices are detected,
inaccessible devices are ignored, with playback (e.g. to the Windows wavetable
synth). It is built easily via *GNU Autotools*, *Qt Creator* or *qmake*, using
*MingW*.  See the INSTALL file for build-from-source instructions for Linux or
Windows, and using a conventional source tarball.

## Recent Changes

    *   Version 0.99.4:
        *   To do.
    *   Version 0.99.3:
        *   Issue #107.  The basic fix is made. Still need to rehabilitate
            the Expand-pattern functionality.
        *   Issue #40.  Improved NSM handling:
            *   Seq66 detects nsmd as the parent process early in startup.
            *   Close ("X") button disabled and hidden.  Xcfe4 "close window"
                action works, though, as does response to a SIGTERM from nsmd.
            *   Handling "config" subdirectory improved.
            *   The --nsm option is now for debugging only, simulating running
                under NSM.
        *   Automation fixes:
            *   Fixed processing grid keystrokes twice.
            *   Renamed record-mode and grid-model control labels.
            *   Fixed accident disabling of grid-mode controls.
            *   Making MIDI control-out handling more reasonable.
            *   Added some LaunchPad Mini macros.
        *   Added reading/writing/displaying Meta text events such as Text
            and Cue Point.
        *   Fixed broken "recent-files" features (by forcing 'rc' save).
        *   Improvements made to playlist handling. It wasn't displaying
            the BPM of the next tune.  Still have issue with the BPM spinbox
            not causing a file modification. Editing the BPM works.
        *   Improvements and important fixes to mute-group and its MIDI
            control still in progress.
        *   Fixed the daemonization and log-file functionality.
        *   Revisited the recmutex implementation.
        *   Weird error where ALSA not found! We now avoid a crash, but
            qseq66 currently exits with only console messages.

    See the "NEWS" file for changes in earlier versions.

C. Ahlstrom 2015-09-10 to 2023-03-27

// vim: sw=4 ts=4 wm=2 et ft=markdown
