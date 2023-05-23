# README for Seq66 0.99.6 2023-05-23

__Seq66__: MIDI sequencer/live-looper with a hardware-sampler grid interface;
pattern banks and triggers, and playlists for song management; a scale and
chord-aware piano-roll; song layout for creative composition; and control/status
via MIDI automation for live performance.  Mute-groups enable/disable multiple
patterns at once.  Supports the Non/New Session Manager; can also run headless.
Works in a space as small as 725x500 pixels (less if window decoration removed).
It does not support audio samples, just MIDI.

__Seq66__ is a major refactoring of Sequencer64/Kepler34/Seq24 with modern C++
and new features.  Linux and Windows users can build this application from
source code.  See the extensive INSTALL file.  Includes a comprehensive PDF
user-manual.

Support sites (still in progress):

    *   https://ahlstromcj.github.io/
    *   https://github.com/ahlstromcj/ahlstromcj.github.io/wiki

The release now includes an installer for the 64-bit Windows version of Seq66.

![Alt text](doc/latex/images/main-window/main-windows.png?raw=true "Seq66")

# Major Features

##  User interface

    *   Qt 5 (cross-platform).  A grid of loop buttons, unlimited external
        windows.  Qt style-sheet support.
    *   Tabs or external windows for management of sets, mute-groups, song
        layout, event-editing, play-lists, and session information.
    *   Low-frequency oscillator (LFO) for modifying continuous controller
        (CC) and velocity values.
    *   A "fixer" for expansion/compression/alignment of note patterns.
    *   Colorable pattern slots; the color palette can be saved and modified.
    *   Horizontal and vertical zoom in the pattern and song editors.
    *   A headless/daemon version can be built.

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

    *   Version 0.99.6:
        *   Added 'o' keystroke to seqroll to toggle recording ('r' already
            used to randomize notes).
        *   Follow-ons to issue #3:
            *   Added a qscrollslave to allow QScrollArea to ignore the
                arrow keys.
            *   This allows the pattern editor pains to remai in sync with
                the seqroll while still allowing use of the hjkl keys. The
                arrow keys work only if the seqroll has keyboard focus.
        *   Follow-ons to issue #110:
            *   Addition of Start menu entries for Windows.
            *   Fixed event::is_desired(), which affected changing note
                velocities in the pattern editor's data pane.
    *   Version 0.99.5:
        *   Greatly enhanced the event editor tab and the events that can
            be view and modified.
        *   Made port-mapping the default. At first startup the map
            exactly matches the existing ports; the user can edit this setup
            in the 'rc' file or the Preferences dialog.
        *   Eliminated "missing ctrl" message at first startup.
        *   Fixed port ID setting in midibus, and adding output flag in
            midi_alsa_info.
        *   Issue #110 Windows: Fixed compiler errors and added scripting to
            build NSIS-based install without leaving Windows, if desired.
        *   Internal refactoring to regularize handling of the session/config
            directory between Linux and Windows.
        *   Fixed portmidi bugs in Linux and Windows, enhanced device naming.
        *   Showing disabled/unavailable MIDI devices as grayed in various
            dropdowns.
        *   Rearranged the Seq66 man pages more sensibly.
    *   Version 0.99.4:
        *   Issue #3: The scroll wheel is enabled in the piano rolls (only).
        *   Issue #48: For a new NSM session configuration, disable "JACK"
            port auto-connect.
        *   Issue #108.  Fixed trying to remove Event tab after deleting it.
        *   Issue #109.  Fixed the application of channels to the various
            export operations.
        *   Fixed minor-but-annoying bug in reporting trying map a "null" buss.
        *   Improved state-appearance of Stop, Pause, and Play buttons.
        *   Fixed issue opening a non-standard-length pattern in its window.
        *   Fixed note events not getting linked after recording.
        *   Fixed drawing of wrap-around notes with linear gradient, and fixed
            handling of note wrap-around when set to false.
        *   Fixed refresh of Mute and Session tabs when loading a MIDI file.
        *   Added seqmenu entry for toggling recording of a pattern.
        *   Added the ability to use the first Text message as "song info".
        *   Seq66 now prevents opening the event editor if recording is in
            progress. Cannot update event editor live with new events.

    See the "NEWS" file for changes in earlier versions.  Some proposed features
    will be pushed off to Seq66v2; see the bottom of the TODO file. Version
    2 is probably a year or two away :-( So many things to improve!

// vim: sw=4 ts=4 wm=2 et ft=markdown
