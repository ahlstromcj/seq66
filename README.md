# README for Seq66 0.97.2

Chris Ahlstrom
2015-09-10 to 2021-11-04

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
    *   Horizontal and vertical zoom in the pattern and song editors.
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
    *   SMF 0 import and export.
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

    *   Version 0.97.2:
        *   Issue #57: Increased width of option fields for wider fonts.
        *   Issue #58: Indicate if NSM is running in Preferences / Sessions and
            disable other session selections at that point.
        *   Issue #59: Spelling error(s) fixed.
        *   Issue #60: Added 'rc' option to disable JACK port auto connect in
            normal mode.  Also present in Preferences / JACK.
        *   Issue #61: Rearranged console output to show the app name, in color.
        *   Issue #63: Initial work on rotating numbering of patterns, sets, and
            mutes, so that numbers vary faster by column than by row.
        *   Issue #64: Fix a bug in optional GUI, testing with Agordejo (NSM).
        *   Added textfix.qss as an example of making disabled text easier to
            read.
        *   Improved the saving/restoring of Edit / Preferences.  Added option
            "Bold Grid Slots" to make the progress bar and progress border thick,
            and the slot font bold.
        *   Added the ability to reload the configuration via a button press
            that restarts the application.
        *   In the live grid, no longer show progress bar moving in unmuted
            tracks.
        *   Mute-group names now stored in the 'mutes' file.  Also able to edit
            them in the Mutes tab.
        *   More streamlining of configuration writing.
    *   Version 0.97.1:
        *   Fixed a bad bug in displaying Notes in the data & event panels in
            the pattern editor, caused by premature ... optimization.
            per issue #61.
        *   Added working tempo-track code to Edit / Preferences / MIDI Clock.
        *   Added exponential ramping of Event Panel events to the pattern LFO
            dialog.
        *   Added the ability to move selected Event Panel events using the
            Left/Right arrow keys.
        *   Improved the handling of sets and external live grids.
        *   Updated documentation extensively. Optimized some images.
        *   Stopped bootstrap --full-clean from removing Makefile.in files.
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
            pattern editor. Also tightened handling of status values.
        *   Can now copy/paste a pattern from one MIDI file to another.
        *   Fixed minor bug in Import dialog handling.
        *   Refactoring for SMF 0 reading and export.
        *   Minor fix to flag changed tune (an asterisk).
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
