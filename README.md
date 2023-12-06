# README for Seq66 0.99.12 2023-12-06

__Seq66__ MIDI sequencer/live-looper with a hardware-sampler grid interface;
pattern banks, triggers, and playlists for song management; scale and chord
aware piano-roll; song layout for creative composition; control/status via MIDI
automation for live performance.  Mute-groups enable/disable sets of patterns.
Supports the Non/New Session Manager; can also run headless.  Works in a space
as small as 450x340 pixels (if window decoration removed).  It does not support
audio samples, just MIDI.

__Seq66__ A major refactoring of Sequencer64/Kepler34/Seq24 with modern C++ and
new features.  Linux and Windows users can build this application from source
code.  See the extensive INSTALL file.  Includes a comprehensive PDF
user-manual.

Support sites (still in progress):

    *   https://ahlstromcj.github.io/
    *   https://github.com/ahlstromcj/ahlstromcj.github.io/wiki

The release now includes an installer for the 64-bit Windows version of Seq66.

![Alt text](doc/latex/images/main-window/main-windows.png?raw=true "Seq66")

# Major Features

##  User interface

    *   Qt 5 (cross-platform).  Loop-button gird. Qt style-sheet support.
    *   Drag-and-drop a MIDI file onto the main grid to load it.
    *   Tabs and external windows for patterns, sets, mute-groups, song
        layout, event-editing, play-lists, and session information.
    *   Low-frequency oscillator (LFO) to modify continuous controller
        and velocity values.
    *   A "fixer" for expansion/compression/alignment of note patterns.
    *   Colorable pattern slots; the color palette can be saved and modified.
    *   Horizontal and vertical zoom in the pattern and song editors.
    *   Extremely resizable.
    *   A headless/daemon version can be built.

##  Configuration files

    *   Supports configuration files: '.rc', '.usr', '.ctrl', '.mutes',
        '.playlist', '.drums' (note-mapping), '.palette', and Qt '.qss'.
    *   Separates MIDI control and mute-group setting into their own files.
    *   Unified keystroke and MIDI controls in the '.ctrl' file; defines MIDI
        controls for automation/display of Seq66 status in grid controllers
        (e.g. LaunchPad).  Sample '.ctrl' files provided for Launchpad Mini.

##  Non/New Session Manager

    *   Support for NSM/New Session Manager, RaySession, Agordejo....
    *   Handles starting, stopping, hiding, and session saving.
    *   Displays details about the session.

##  Multiple Builds

    *   ALSA/JACK: `qseq66` using an rtmidi-based library
    *   Command-line/headless: `seq66cli`
    *   PortMidi: `qpseq66`
    *   Windows: `qpseq66.exe`

##  More Features

    *   Supports configurable PPQN from 32 to 19200 (default is 192).
    *   Transposable triggers to re-use patterns more comprehensively.
    *   Song import/export from/to stock MIDI (SMF 0 or 1).
    *   Highly configurable MIDI-based metronome.
    *   Improved non-U.S. keyboard support.
    *   Many demonstration and test MIDI files.
    *   See the **NEWS** file or **RELNOTES**.

##  Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce polling.
    *   A ton of clean-up and refactoring.

Seq66 uses a Qt 5 user-interface based on Kepler34 and the Seq66 *rtmidi*
(Linux) and *portmidi* (Windows) engines.  MIDI devices are detected,
inaccessible devices are ignored, with playback (e.g. to the Windows wavetable
synth). It is built easily via *GNU Autotools*, *Qt Creator* or *qmake*, using
*MingW*.  See the INSTALL file for build-from-source instructions for Linux or
Windows, and using a conventional source tarball.

## Recent Changes

    As per Issue #123 "Would it be possible to have NEWS structured like
    a changelog", the recent changes now appear at the top of the NEWS file.
    Also see RELNOTES.

// vim: sw=4 ts=4 wm=2 et ft=markdown
