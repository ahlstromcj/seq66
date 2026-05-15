# README for Seq66 0.99.25 2026-05-15

__Seq66__ MIDI sequencer/live-looper with a hardware-sampler grid interface;
pattern banks, triggers, and playlists for song management; scale and chord
aware piano-roll; song layout for creative composition; control/status via MIDI
automation, and mute-groups to enable/disable sets of patterns. Tools for live
performance and for composing great-sounding MIDI tracks. Supports the Non/New
Session Manager; can run headless and on a small computer like the Pi.  It does
not support audio samples, just MIDI.

__Seq66__ Seq24/Kepler34 on steroids with
modern C++ and new features. Linux and Windows users can build this application
from source code. See the extensive INSTALL files. Includes a comprehensive PDF
user-manual. As of this release, employs the __Meson__ build system.

*The current development-in-progress branch is now "Meson". Seq66 now
builds using Meson, and supports Qt6. Bootstrap and build now done via
the work.sh script. The old-style build setup is preserved in branch
"Autoconf."*

The release includes an installer for the 64-bit Windows version of
Seq66. Initial work has been done on getting Seq66 to build and run
in FreeBSD using the Clang compiler. A C++17 capable-compiler is
needed.

See NEWS for updates and RELNOTES for the latest highlights.

The figure below shows Seq66 with modified palette and a style-sheet
in force. Otherwise the application uses the current Qt theme.

![Alt text](doc/latex/images/main-window/main-windows-perstfic.png?raw=true "Seq66")

# Features

##  User interface

    *   Qt 5 or Qt 6 (cross-platform). Loop-button grid. Qt style-sheet
        support.
    *   Colorable pattern slots; the palette can be saved in a '*.palette' file.
    *   Drag-and-drop a MIDI file onto the main grid to load it.
    *   Tabs and external windows for patterns, sets, mute-groups, song
        layout, event-editing, play-lists, and session information.
    *   Low-frequency oscillator (LFO) to modify continuous controller
        and velocity values.
    *   A "fixer" for expansion/compression/alignment of note patterns.
    *   Horizontal and vertical zoom in the pattern and song editors.
    *   Extremely resizable.
    *   A headless/daemon version can be built to use with a MIDI grid
        controller.

##  Configuration files

    *   Supports configuration files: '.rc', '.usr', '.ctrl', '.mutes',
        '.playlist', '.drums' ('.notemap'), '.palette', and Qt '.qss'.
    *   Separates MIDI control and mute-group setting into their own files.
    *   Unified keystroke and MIDI controls in the '.ctrl' file; defines MIDI
        controls for automation/display of Seq66 status in grid controllers
        (e.g. LaunchPad). Sample '.ctrl' files provided for Launchpad Mini.

##  Non/New Session Manager

    *   Support for NSM/New Session Manager, RaySession, Agordejo....
    *   Handles starting, stopping, hiding, and session saving.
    *   Displays details about the session.

##  Multiple Builds

    *   Meson:
        *   ALSA/JACK: `qseq66` using an rtmidi-based library
        *   PortMidi: `qseq66` using a portmidi-based library
        *   Command-line/headless: `seq66cli`
    *   qmake:
        *   PortMidi: `qpseq66`
        *   Windows: `qpseq66.exe`

##  More Features

    *   Supports configurable PPQN from 32 to 19200 (default is 192).
    *   Transposable triggers to re-use patterns more comprehensively.
    *   Song import/export from/to stock MIDI (SMF 0 or 1).
    *   Highly configurable MIDI-based metronome.
    *   Management of scales, keys, and chords.
    *   Improved non-U.S. keyboard support.
    *   Many demonstration and test MIDI files.

##  Internal

    *   More consistent use of modern C++, auto, and lambda functions.
    *   Additional performer callbacks to reduce polling.
    *   A ton of clean-up and refactoring.

Seq66 has a user-interface based on Kepler34 and the Seq66 *rtmidi* (Linux)
and *portmidi* (Windows) engines. MIDI devices are detected, inaccessible
devices are ignored, with playback (e.g. to the Windows wavetable synth).
It is built on "linux" via *Meson*, *Qt Creator* or *qmake*, and on
Windows using *MingW*.

*IMPORTANT*: *GNU Autotools* is no longer supported, except in the "Autoconf"
branch.

The INSTALL has been streamlined, see that file for build-from-source
instructions for Linux or Windows, and using a conventional source tarball.

Support sites (still in progress):

    *   https://ahlstromcj.github.io/
    *   https://github.com/ahlstromcj/ahlstromcj.github.io/wiki

## Recent Changes

    *   See the NEWS file.

// vim: sw=4 ts=4 wm=2 et ft=markdown
