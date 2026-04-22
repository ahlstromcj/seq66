
# INSTALL Seq66 for Ubuntu Studio

Authors: WinkoErades and MQS-mark <p>
Updated: 2026-01-16

Here’s a **step-by-step installation plan** to build and install
**Seq66** from source on **Ubuntu Studio 25.10**.
This assumes one is *installing from the GitHub repository*:
[Seq66 GitHub](https://github.com/ahlstromcj/seq66/tree/master)
([GitHub](https://github.com/ahlstromcj/seq66?utm_source=chatgpt.com
"Seq66: Seq24-based live MIDI looper/editor. v. ..."))

## 1. Prepare Ubuntu Studio 25.10

Ubuntu Studio is already tailored for audio production, but ensure
your system is up to date:

```bash
sudo apt update && sudo apt upgrade -y
```

Reboot if needed after upgrades.

## 2. Install Required Build Dependencies

Seq66 uses **GNU Autotools** and **Qt5** for the GUI version,
plus OSC/MIDI support. Required development tools and libraries:

```bash
sudo apt install -y \
    autoconf \
    automake \
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
    libudev-dev
    pkg-config \
    qt5-qmake \
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
```

*Explanation:*

- `build-essential`, `automake`, `autoconf`, etc., provide the core
  build toolchain.
- `qtbase5-dev` and related packages supply Qt5 headers/tools for
  building the Seq66 GUI.
- `libasound2-dev`, `libjack-jackd2-dev`, `liblo-dev` cover ALSA,
  JACK/pipewire jack compatibility, and liblo (OSC), all typical
  MIDI/audio support libs.
- `libatopology-dev` might not be needed, depending on the Ubuntu version.

(Exact package names can vary slightly in Ubuntu 25.10; `apt search liblo`
etc. helps confirm.)
([INSTALL](https://github.com/ahlstromcj/seq66/blob/master/INSTALL))

## 3. Clone the Seq66 Repository

Choose a location such as your home directory and clone the latest code:

```bash
cd ~
git clone https://github.com/ahlstromcj/seq66.git
cd seq66
```

Make sure you’re on the **master** branch (default) unless you want a different
release.

## 4. Bootstrap and Configure

Some older Seq66 releases may require regenerating the build system if `configure`
is missing or outdated:

```bash
./bootstrap
```

If that script is not present or errors, use `autoreconf`:

```bash
autoreconf --install --force
```

Now configure the build:

```bash
./configure
```

You can pass optional flags if needed, e.g.:

- `--enable-rtmidi` (explicitly enable RtMidi backend)
- `--disable-portmidi` (if portmidi isn’t installed)
    *(Check `./configure --help` for options.)*
    Note: `configure` will detect Qt5 if installed.
    ([Seq66 GitHub](https://github.com/ahlstromcj/seq66?utm_source))

📌 *Common issues*: If `./configure` fails complaining about missing files
like `Makefile.in`, ensure you ran `autoreconf` first.
Older seq66 sources may need that.
([Issue 25](https://github.com/ahlstromcj/seq66/issues/25))

## 5. Build Seq66

Compile the code:

```bash
make -j$(nproc)
```

`-j$(nproc)` speeds up compilation using all CPU cores. If you only want the
command-line version, you can compile only that target after configure:

```bash
make seq66cli
```

## 6. Install

Once built, install it system-wide:

```bash
sudo make install
```

This typically installs binaries like **qseq66**, **qpseq66** (Qt GUI),
and **seq66cli** into `/usr/local/bin` or similar.

## 7. Optional: Add to Desktop Menu

If the GUI version built (`qseq66` or `qpseq66`), a `.desktop` file already exists
in the repo’s `desktop` folder. You can install it so it appears in your application
launcher:

```bash
sudo cp desktop/seq66.xpm /usr/share/applications/
```

Then update desktop DB:

```bash
sudo update-desktop-database
```

## 8. Run and Test

Launch either:

- **GUI**: `qseq66` or `qpseq66`

- **CLI**: `seq66cli`

Ensure your MIDI devices are connected and recognized. Ubuntu Studio’s audio tools
(like QJackCtl or Ubuntu Studio Audio Config) may help routing JACK/ALSA.

## Troubleshooting Tips

### Missing `configure` or autotools error

- Run `autoreconf --install --force` before `./configure`.
  ([Issue 25](https://github.com/ahlstromcj/seq66/issues/25"))

- Ensure `automake`, `autoconf`, `libtool` are installed.

- If the system complains about not having Autoconf >= 2.72 installed,
  edit `configure.ac` to reduce the `AC_PREREQ` version number to what is
  actually installed, and start over.

### Qt not detected

- Confirm `qtbase5-dev`, `qttools5-dev`, `qttools5-dev-tools`, and `qmake`
  are installed.

- Use `qtchooser -list-versions` to check available Qt versions.

### Missing libs (such as liblo)

- Install `liblo-dev` (for OSC) before configure.
  ([Reddit Seq66](https://www.reddit.com/r/sequencers/comments/16o7b7t/seq66_debian_install/))

### Build fails due to ALSA/JACK

- Check `libasound2-dev` and appropriate JACK dev packages are installed.

## Useful References

- Official Seq66 **README / INSTALL** in the repository (detailed build
  instructions).
  ([Seq66 GitHub](https://github.com/ahlstromcj/seq66))

- User manual and tutorials on the project site (great for usage after
  install).
  ([Seq66 Manual](https://raw.githubusercontent.com/ahlstromcj/seq66/master/data/share/doc/seq66-user-manual.pdf))

[//] vim: sw=4 ts=4 wm=2 et ft=markdown
