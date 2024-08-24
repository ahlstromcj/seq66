# ROADMAP for a Possible Seq66 v. 2

Chris Ahlstrom
2022-05-11 to 2022-08-24

This file is simply some thoughts about the future of __Seq66__ and
a version 2...  if we decide to keep updating this project until Ahlstrom
croaks. :-D Also see the bottom of the TODO file.

# Topics

##  Qt Upgrade

    It should be straightforward to upgrade from Qt 5 to Qt 6.  We will see if
    it is better to leverage The Qt5Compat library at first.

##  Configuration files

    Currently, we see no need to change the format of the Seq66 configuration
    files greatly.  The INI format is simpler and easier to read and edit than
    XML, and covers our needs well.

##  Non Session Manager

    Will continue to be supported.  Will remove the unused nsmserver module,
    which was mostly meant for "just in cases" already covered by other
    NSM-derived projects.

##  Builds

    *   We might transition to Cmake, based on its cross-platform support.
        Or not, as there are also a lot of complaints about it, even lately.
        It might be nice to stick with Qmake, but it's been deprecated by Qt.
        There should be no need for both the build systems we use at present:
        Autotools and Qmake.
    *   The libraries libseq66, sessions, seq\_portmidi, and seq\_rtmidi will
        be split into separate projects for better re-use.
    *   A new namespace would be in order:  seqx or somesuch.

##  Engines

    *   Remove the Mac and Windows support from our derivative "portmidi" library.
    *   Move the support to our derivative "rtmidi" library.

##  Executables

    The following executables will still be supported, though with new names and
    features.

    *   ALSA/JACK: `qseq66`
    *   Command-line/headless: `seq66cli`
    *   Windows: `qpseq66.exe`

## Code Improvements

    *   Untangle some of the header-file dependencies.
    *   Create independent libraries for strings, files, midi, etc.
    *   Add comprehensive Qt string translation.

##  Additional Features

    *   Limited support for audio patterns.  Clips that could compress or
        expand to fit the BPM and measures and be included in the MIDI file in a
        new SeqSpec section.  However, editing would be left to far better audio
        applications.  Perhaps best supported as a soundfont?
    *   A comprehensive and improved system for undo/redo support for
        configuration and for all song changes.
    *   Use "rtmidi" for Windows/Mac; removed "portmidi".

##  Requested Features / Suggestions

    These items are requests.  Not sure if all are worth doing in version 2.

    *   Replace GNU Autotools.  Candidates: meson and cmake.  Meson seems
        preferable at this time.
    *   Use a true RtMidi-compatible library; dump the portmidi implementation.
    *   Add Pipewire support.
    *   Full OSC support for automation.
    *   Break libraries into Git submodules.
    *   MIDI clips launched by a note.
    *   MIDINAM.  See issue #1 and the TODO file for this request.
    *   Ableton Live transport support.  See issue #16 and the TODO file for
        this request.
    *   Support RELNOTEs.md better, as per issue #24 and the TODO.
    *   Beef up recording live sequence changes as per issue #44 and the TODO.
    *   Weak JACK.

// vim: sw=4 ts=4 wm=2 et ft=markdown
