# Seq66 Release Notes 0.98.6
==============================
Chris Ahlstrom
2020-11-15 to 2022-03-29

This file lists only the major changes for this version from the previous
version.  Also see the README.md, ChangeLog, NEWS, and INSTALL files.

## Feature List

    *   Non Session Manager (NSM) protocol support.
    *   JACK Session support. Deprecated, but still useful for some.
    *   JACK Metadata Support. Works on systems with a version of JACK
        that supports it. JACK aliases also supported if available.
    *   Port mapping. Each pattern holds a port number, which can be mapped to
        a specific system MIDI device or application.
    *   Mute groups. Enable/disable a complete set of patterns at once.
    *   Playlist. Allow easy navigation between tunes in a list.
    *   Windows build. Uses an internal implementaton of PortMidi. NSIS
        installer.
    *   Automation. Allows a MIDI device to be used for control of most
        functions. Status displays to a MIDI device as well.
    *   Support for SMF 0 and SMF 1 files, plus sequencer-specific information.

## Documentation

LaTeX/pdflatex is used for generation of the PDF manual in "data/share/doc".  To
rebuild this document, change the the "doc/latex" directory and run "make".  The
file "seq66-user-manual.pdf" is generated in the "data/share/doc" directory.

## Final Notes

All too many bug fixes and minor improvements.  Never-ending!

/*
 * vim: sw=4 ts=4 wm=4 et ft=markdown
 */
