# INSTALL Seq66 for Ubuntu Studio

Authors: WinkoErades and MQS-mark <p>
Updated: 2026-05-08

Here’s a **step-by-step installation plan** to build and install **Seq66** from source on **Ubuntu Studio 25.10**. This assumes you are *installing from the GitHub repository* you linked: **https://github.com/ahlstromcj/seq66/tree/master** ([GitHub](https://github.com/ahlstromcj/seq66?utm_source=chatgpt.com "Seq66: Seq24-based live MIDI looper/editor. v. ..."))

## 1. Prepare Ubuntu Studio 25.10

Ubuntu Studio is already tailored for audio production, but ensure your system is up to date:

```bash
sudo apt update && sudo apt upgrade -y
```

Reboot if needed after upgrades.

## 2. Install Required Build Dependencies

Seq66 used to use **GNU Autotools** and **Qt 5** for the GUI version, plus
OSC/MIDI support. Now it replaces **Autotools** with the **Meson** build system,
and also supports **Qt 6** by default.
You need development tools and libraries (replacing qt5 by qt6 if desired):

```bash
sudo apt install -y \
    build-essential \
    git \
    libasound2-dev \
    libatopology-dev \
    libc6-dev \
    libjack-jackd2-dev \
    liblo-dev \
    libpcre2-dev \
    libqt5widgets5 \
    libtool \
    libudev-dev \
    meson \
    ninja \
    pkg-config \
    qt5-qmake \
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
```

*Explanation:*

- `build-essential`, `meson`, and `ninja`, etc., provide the core
  build toolchain.
- `qtbase5-dev` and related packages supply Qt5 headers/tools for
  building the Seq66 GUI. Or install teh Qt6 tools.
- `libasound2-dev`, `libjack-jackd2-dev`, `liblo-dev` cover ALSA,
  JACK/pipewire jack compatibility, and liblo (OSC), all typical
  MIDI/audio support libs.
- `libatopology-dev` might not be needed, depending on the Ubuntu version.

    (Exact package names can vary slightly in Ubuntu 25.10; `apt search liblo` etc. helps confirm.) ([GitHub](https://github.com/ahlstromcj/seq66/blob/master/INSTALL?utm_source=chatgpt.com "seq66/INSTALL at master"))

## 3. Clone the Seq66 Repository

Choose a location such as your home directory and clone the latest code:

```bash
cd ~
git clone https://github.com/ahlstromcj/seq66.git
cd seq66
```

Make sure you’re on the **master** branch (default) unless you want a different release. Note that the **Autoconf** branch is semi-frozen to preserve the old style of build.

## 4. Bootstrap and Configure

The **bootstrap** script has been replaced by the **work.sh** script, which
encapsulates a number of Meson build commands. Of course, experience Meson users can work without using this script.

The work.sh script can restart the whole build configuration:

```bash
    $ ./work.sh --clean
```

Note that 'work.sh' has a '--help' option.

Now configure the build and make it (can add the --debug option if desired).

```bash
    $ ./work.sh
```
This also builds the project. To just configure it, add the --setup option. By default, all build products are put in ./build/cc.

You can pass optional flags if needed:

- `--release` to build in release mode (the default).
- `--debug` to build in debug mode.
- `--clang` to use the Clang compilers instead of the system's default compiler. Build products go into ./build/clang.
- `--gnu` to use the GNU compilers instead of the system's default compiler. Build products go into ./build/gcc.
- `--portmidi` to enable that code; it lacks a couple features of the normal build.
- `--build dir` to use a different build directory. If used, must be used for follow-on work.sh commands.
- `--clean` to remove all build products.
- `--install` to install qseq66 and seq66cli and the icons and samples.
- `--uninstall` to uninstall the build status.
    *(Check `./work.sh --help` for options.)*  
    Note: `Meson` will detect Qt6 if installed; Qt5 can also be used. ([GitHub](https://github.com/ahlstromcj/seq66?utm_source=chatgpt.com "Seq66: Seq24-based live MIDI looper/editor. v. ..."))

📌 *Common issues*: If `./configure` fails complaining about missing files like `Makefile.in`, ensure you ran `autoreconf` first. Older seq66 sources may need that. ([GitHub](https://github.com/ahlstromcj/seq66/issues/25?utm_source=chatgpt.com "Configure fails · Issue #25 · ahlstromcj/seq66"))

## 5. Build Seq66

Compile the code if not done earlier:

```bash
./work.sh
```

## 6. Install

Once built, install it system-wide:

```bash
sudo ./work.sh --install
```

This typically installs binaries like **qseq66** (Qt GUI), and **seq66cli** into `/usr/local/bin` or similar.

## 7. Add to Desktop Menu

When the GUI is built (`qseq66`), a `.desktop` file is installed automatically so it appears in your application launcher:

```bash
sudo cp data/share/applications/seq66.desktop /usr/share/applications/
```

Update desktop database after the installation:

```bash
sudo update-desktop-database
```

## 8. Run and Test

Launch either:

- **GUI**: `qseq66`
- **CLI**: `seq66cli`

Ensure your MIDI devices are connected and recognized. Ubuntu Studio’s audio tools (like QJackCtl or Ubuntu Studio Audio Config) may help routing JACK/ALSA.

## Troubleshooting Tips

**Missing `configure` or autotools errors**

- Ensure `meson`, `ninja`, etc. are installed.

**Qt not detected**

- Confirm `qtbase5-dev`, `qttools5-dev`, `qttools5-dev-tools`, and `qmake` are installed. Make sure your environment sets something like "export QT\_SELECT=qt6.
- Use `qtchooser -list-versions` to check available Qt versions.
    
**Missing libs (such as liblo)**

- Install `liblo-dev` (for OSC) before configure. ([Reddit](https://www.reddit.com/r/sequencers/comments/16o7b7t/seq66_debian_install/?utm_source=chatgpt.com "Seq66 Debian install : r/sequencers"))

**Build fails due to ALSA/JACK**

- Check `libasound2-dev` and appropriate JACK dev packages are installed.

## Useful References

- Official Seq66 **README / INSTALL** in the repository (detailed build instructions). ([GitHub](https://github.com/ahlstromcj/seq66?utm_source=chatgpt.com "Seq66: Seq24-based live MIDI looper/editor. v. ..."))
    
- User manual and tutorials on the project site (great for usage after install). ([GitHub](https://raw.githubusercontent.com/ahlstromcj/seq66/master/data/share/doc/seq66-user-manual.pdf?utm_source=chatgpt.com "Seq66 User Manual v. 0.99.22 - GitHub"))

[//] vim: sw=4 ts=4 wm=2 et ft=markdown
