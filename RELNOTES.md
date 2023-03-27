# Seq66 Release Notes 0.99.3 2023-03-28

This file lists __major__ changes from version 9.99.1 to 0.99.3 (to catch up).

## Changes

    *   Version 0.99.3:
        *   Issue #107.  Expand-pattern functionality.
        *   Automation fixes.
        *   Fixed the daemonization and log-file functionality.
    *   Version 0.99.2:
        *   Issue #103.  Some improvements to pattern loop-count.
        *   Pull request #106. User phuel added checkmarks for active buss and
            channel in grid-slot menu.
        *   Fixed background sequence not displaying when running with
            linear-gradient brush.
        *   Fixes to brushes; made the linear gradient (notes and triggers)
            a default run-time option.
        *   Other minor fixes and documentation updates, including the manual.
    *   Version 0.99.1:
        *   Issue #44. Revisited to fix related additional issues. Can now
            toggle a pattern's song record in perfnames. Record button:
            Ctrl disables snap, Shift enables record at playback start.
        *   Issue #93. Revisited to fix related open pattern-editor issues.
        *   Issue #100. Partly mitigated. Added a custom ringbuffer for MIDI
            message objects to replace JACK's ringbuffer.
        *   Various fixes:
            *   Fixed partial breakage of pattern-merge function.
            *   Fixed odd breakage of ALSA playback in release mode.
            *   Fixed Stop button when another Master has started playback.
            *   Shift-click on Stop button rewinds JACK transport when running
                as JACK Slave.
            *   Display of some JACK server settings in Edit / Preferences.
            *   Fixed handling of Ctrl vs non-Ctrl zoom keys in perfroll.
            *   Event-dump now prompts for a text-file name.
        *   Added linear-gradient compile-time option for displaying notes
            and triggers.

## Final Notes

Read the NEWS, README, and TODO files.  Never-ending!

