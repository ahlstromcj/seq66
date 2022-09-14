# README for Seq66 0.99.1

Chris Ahlstrom
2015-09-10 to 2022-09-14

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

For version 0.99.0, a raft of updates and fixes were made as we work through
some of the items in the TODO file. This version series will add no new major
features, but will follow up on the remaining issues and any new issues that
come up. New features will be pushed off to Seq66v2; see the bottom of the TODO
file.

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
    *   New. An editor for expansion/compression/alignment of note patterns.
    *   Each pattern slot can be colored; the color palette for slots and
        the piano rolls can be saved and modified.
    *   Horizontal and vertical zoom in the pattern and song editors.
    *   A headless version can be built.

##  Configuration files

    *   Separates MIDI control and mute-group setting into their own files.
    *   Supports configuration files: ".rc", ".usr", ".ctrl", ".mutes",
        ".playlist", ".drums" (note-mapping), ".palette", and Qt ".qss".
    *   Unified keystroke and MIDI controls in the ".ctrl" file; defines MIDI
        controls for automating Seq66 and for displaying Seq66 status in grid
        controllers (e.g. LaunchPad).  Basic ".ctrl" files are provided for the
        Launchpad Mini.

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

    *   Transposable triggers to re-use patterns more comprehensively.
        Works with Song Export.
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

    *   Version 0.99.1:
        *   Issue #93. Revisited to fix related open pattern-editor issues.
        *   Issue #100. In progress; works but still can overflow buffer under
            heavy playback.
        *   Various fixes:
            *   Fixed partial breakage of pattern-merge function.
            *   Fixed odd breakage of ALSA playback in release mode.
            *   Fixed Stop button when another Master has started playback.
    *   Version 0.99.0:
        *   Issue #44. Record live sequence changes functionality beefed up
            to handle recording without snapping.
        *   Issue #54. Updated the ax_have_qt_min.m4 file to detect
            qmake-qt5, etc.
        *   Issue #78 revisited. Pattern-box sizes would become 0 and the
            progress boxes disappear. Now the 'usr' show option is boolean
            ("pattern-boxes-shown", default = true), and the sizes are kept
            within reasonable limits.  Also added a "--locale" option so that
            the user can, for example, set the Seq66 global locale to
            "en_US.UTF-8".
        *   Issue #82 allows buttons and fields to expand better.
            Fixed for main window and the song & pattern editors only.
        *   Issue #89 fixed. The MIDI control display not quite reflecting the
            status of each pattern, especially during queuing.
        *   Issue #90 improvements. Save was not always enabled. Surely some
            issues remain.  Also, in some cases there is no way to "unmodify".
        *   Issue #93 fixed. The window of a deleted pattern now closes.
        *   Issue #94. Long song in song editor could not be scrolled to the
            right.  Added more padding and a button to expand the grid when
            when desired. For the pattern editor, the workaround is to increase
            the "length" in the pattern editor.
        *   Issue #97. Investigate/resolve differences from Seq24.
            *   Pattern editor fixes.
            *   Added paste box when pasting notes, an oversight from the Seq24
                reboot.
            *   Added Ctrl-Left/Right to move the progress bar in the pattern
                editor. (Left/Right scrolls the piano roll.)
        *   Issue #98. Feature requests (metronome and background recording).
            *   Added an initial metronome facility and 'rc' configuration.
            *   Metronome count-in added.
            *   Background automatic recording added.
        *   Various fixes:
            *   Muted pattern slots show a short progress bar, to aid in the
                timing of queuing.
            *   Improved the handling of the MIDI 'ctrl' file and control
                states upon a restart.
            *   Tightened up pattern arming/disarming processing.
            *   Implemented left/right arrow keys to move the selected trigger
                in the song editor. Ctrl moves multiple triggers. Moving a
                trigger past END moves END.
            *   Fixed error in painting tempo events in triggers (perfroll).
            *   Improved keystroke movement of "L"/"R" markers in song and
                pattern editor time bars.
            *   The global time signature is now applied to new patterns.
            *   Added a try-catch to showing the locale.
            *   Ctrl-Z removes all mouse-painted notes at once (like Seq24).
                Single-note removal is macroed out.
            *   Error in parsing "--option sets=RxC" fixed.
            *   Improved the handling of grid font sizes re window size.
            *   Fixed the response to incoming MIDI Continue.
            *   PPQN != 192 : handle snapping when adding notes; bugs in
                perfroll and perftime.
            *   Implement clear-events and double-length grid modes.

    See the "NEWS" file for changes in earlier versions.

// vim: sw=4 ts=4 wm=2 et ft=markdown
