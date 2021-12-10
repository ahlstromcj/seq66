readme.txt for Seq66 0.98.0
Chris Ahlstrom
2015-09-10 to 2021-12-11

Seq66 is a refactoring of a reboot (sequencer64) of seq24, extending it with new
features and bug fixes, and incorporation of "Modern C++" (C++11 or C++14).  It
is a "live performance" sequencer, with the musician creating and controlling a
number of pattern loops.

An extensive manual for this application is found in the "doc" subdirectory
of the project directory, which can be downloaded via Git:

    https://github.com/ahlstromcj/seq66.git

It is also installed in "/usr/local/share/doc/seq66-0.97" (or a later version
number) along with ODF spreadsheets describing the control keys and Launchpad
Mini configuration.  It covers everything that seq66 can do.
    
Prebuilt Debian packages, Windows installers, and source tarballs are
available here, in the project directory for Sequencer64 packages:

    https://github.com/ahlstromcj/sequencer64-packages.git

See the seq66/0.97 (or latest version) directory for the Windows installer.

Windows support:

    This version uses a Qt 5 user-interface based on Kepler34, but using the
    standard Seq66 libraries.  The user-interface works well, and Windows
    built-in MIDI devices are detected, inaccessible devices are ignored, and
    playback (e.g. to the built-in wavetable synthesizer) work.
    The Qt 5 GUI removes some of the overly-convoluted features of the
    Gtkmm 2.4 GUI, while adding a mutes-master and a set-master tab.
    It is about 99% complete and very useable.

    Currently, manual configuration of the "rc" and "usr" files is necessary.
    Also supported are separate "ctrl" (MIDI control, input and output),
    "mutes" (toggles of patterns of sequences), "drums" (note mappings) and
    "playlist" files.  See the READMEs for more information.

    See the file C:\Program Files(x86)\Seq66\data for README.windows,
    which explains some things to watch for with Windows.

See the INSTALL file for build-from-source instructions or using a
conventional source tarball.  This file is part of:

    https://github.com/ahlstromcj/seq66.git

# vim: sw=4 ts=4 wm=4 et ft=sh fileformat=dos
