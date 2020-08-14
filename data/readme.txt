readme.txt for Seq66 0.90.5
Chris Ahlstrom
2015-09-10 to 2020-08-14

Seq66 is a refactoring of a reboot of seq24, extending it with new features
and bug fixes, and incorporation of "Modern C++" (C++11 or C++14).  It is a
"live performance" sequencer, with the musician creating and controlling a
number of pattern loops.

An extensive manual for the original is found at:

    https://github.com/ahlstromcj/sequencer64-doc.git

It covers most of what seq66 can do, and will have to suffice for now.  We
will be working on the new manual, which will be included in the source-code
project.
    
Prebuilt Debian packages, Windows installers, and source tarballs will
(eventually) be available here, in the project directory for Sequencer64
packages:

    https://github.com/ahlstromcj/sequencer64-packages.git

Windows support:

    This version uses a Qt 5 user-interface based on Kepler34, but using the
    standard Seq66 libraries.  The user-interface works well, and Windows
    built-in MIDI devices are detected, inaccessible devices are ignored, and
    playback (e.g. to the built-in wavetable synthesizer) work.
    The Qt 5 GUI removes some of the overly-convoluted features of the
    Gtkmm 2.4 GUI, while adding a mutes-master and a set-master tab.
    It is about 99% complete and very useable.

    Currently, manual configuration of the "rc" and "usr" files is necessary.
    Also supported are separate "ctrl" (MIDI control, input and output) and
    "mutes" files.  See the READMEs for more information.

    See the file C:\Program Files(x86)\Seq66\data for README.windows,
    which explains some things to watch for with Windows.

See the INSTALL file for build-from-source instructions or using a
conventional source tarball.  This file is part of:

    https://github.com/ahlstromcj/seq66.git

# vim: sw=4 ts=4 wm=4 et ft=sh fileformat=dos
