# README for Seq66 0.99.11 2023-11-06

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
    *   Improved non-U.S. keyboard support.
    *   Many demonstration and test MIDI files.
    *   See **Recent Changes** below, and the **NEWS** file.

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

    *   Version 0.99.11:
        *   Added 8 more ui-palette entries, total of 32. Probably enough.
        *   Fixed errors specifying style-sheet, palette, and mutes in
            Preferences / Session.  Also enhanced this tab to indicate
            exit is needed.
        *   Fixing various playlist errors:
            -   PPQN setting issue causing slow/fast playback.
            -   Segfaults due to not stopping playback before loading
                the next song or basing calculations on missing values..
        *   Fixed app exiting unceremoniously if "quiet" is set.
        *   Fixed minor issue in Song zoom with 1920 PPQN.
        *   Focus is now set immediately to the seqroll and perfroll.
        *   Replaced some verbose() checks with investigate() checks to cut
            down on console output. In the CLI, --verbose now shows
            playlist actions on the console (it prevent daemonization and
            logging to a file).
        *   Moved the style-sheet options from the 'usr' file to the 'rc'
            file with all the rest of the file specifications. The upgrade
            is automatic.
        *   Replaced the --inspect option with --session-tag to allow easy
            changing to another setup specified in sessions.rc.
        *   Added mute-group label to main window.
    *   Version 0.99.10:
        *   Issue #117 Option to close pattern windows with esc key. Must
            be enabled via a 'usr' option first.
        *   Issue #118 Make virtual ports ports enabled by default.
        *   Issue #119 "Quantized Record Active does not work" fixed.
            Note-mapping also fixed with this issue.
        *   Fixed an egregious error in drawing notes in drum mode.
        *   Fixed error in moving notes at PPQN != 192.
        *   Added drag-and-drop of one MIDI file onto the Live grid.
        *   Added the export of most project configuration files to another
            directory.
        *   Multiple tempo events can be drawn in a line in the data pane.
            They can be dragged up and down in the data pane.
        *   If configured for double-click, can now open or create a pattern
            by double-clicking in the song editor(s).
        *   Improved modification detection in the data pane.
        *   Many improvements and fixes to the Mutes tab.
        *   Made 'rc' file-name handling more robust.
        *   Added a new "grid mode" to allow toggling mutes by clicking
            in the Live Grid.  The default group-selection keystroke is "_".
        *   Tweaked the main time display to work better with high PPQN.
        *   Added feature to scroll automatically in time and note value
            to show the first notes in a pattern.
        *   The live-grid record-mode and alteration statuses are now applied
            when the pattern editor is opened. The "Record toggle" popup
            menu entry and MIDI control also support this functionality.
        *   Added more UI palette items for more non-Qt color control.
            Modified the inverse color palette slightly. Added checkmark
            for the active pattern color in its popup menu.
        *   Tightened the file-name/activity handling in Session preferences.
        *   Restored missing table header in Mutes tab.

    See the "NEWS" file for changes in earlier versions.  Some proposed features
    will be pushed off to Seq66v2; see the bottom of the TODO file. Version 2 is
    probably a year or two away :-( So many things to improve!

// vim: sw=4 ts=4 wm=2 et ft=markdown
