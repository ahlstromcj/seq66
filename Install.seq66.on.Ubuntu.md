Here’s a **step-by-step installation plan** to build and install **Seq66** from source on **Ubuntu Studio 25.10**. This assumes you are *installing from the GitHub repository* you linked: **https://github.com/ahlstromcj/seq66/tree/master** ([GitHub](https://github.com/ahlstromcj/seq66?utm_source=chatgpt.com "Seq66: Seq24-based live MIDI looper/editor. v. ..."))

## 1. Prepare Ubuntu Studio 25.10

Ubuntu Studio is already tailored for audio production, but ensure your system is up to date:

```bash
sudo apt update && sudo apt upgrade -y
```

Reboot if needed after upgrades.

## 2. Install Required Build Dependencies

Seq66 uses **GNU Autotools** and **Qt5** for the GUI version, plus OSC/MIDI support. You need development tools and libraries:

```bash
sudo apt install -y \
    build-essential \
    git \
    automake \
    autoconf \
    libtool \
    pkg-config \
    qtbase5-dev \
    qttools5-dev-tools \
    qttools5-dev \
    libqt5widgets5 \
    libasound2-dev \
    libjack-jackd2-dev \
    liblo-dev \
    libc6-dev \
    libpcre2-dev \
    qt5-qmake \
    libudev-dev
```

*Explanation:*

- `build-essential`, `automake`, `autoconf`, etc., provide the core build toolchain.
    
- `qtbase5-dev` and related packages supply Qt5 headers/tools for building the Seq66 GUI.
    
- `libasound2-dev`, `libjack-jackd2-dev`, `liblo-dev` cover ALSA, JACK/pipewire jack compatibility, and liblo (OSC), all typical MIDI/audio support libs.  
    (Exact package names can vary slightly in Ubuntu 25.10; `apt search liblo` etc. helps confirm.) ([GitHub](https://github.com/ahlstromcj/seq66/blob/master/INSTALL?utm_source=chatgpt.com "seq66/INSTALL at master"))
    

## 3. Clone the Seq66 Repository

Choose a location such as your home directory and clone the latest code:

```bash
cd ~
git clone https://github.com/ahlstromcj/seq66.git
cd seq66
```

Make sure you’re on the **master** branch (default) unless you want a different release.

## 4. Bootstrap and Configure

Some older Seq66 releases may require regenerating the build system if `configure` is missing or outdated:

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
    Note: `configure` will detect Qt5 if installed. ([GitHub](https://github.com/ahlstromcj/seq66?utm_source=chatgpt.com "Seq66: Seq24-based live MIDI looper/editor. v. ..."))
    

📌 *Common issues*: If `./configure` fails complaining about missing files like `Makefile.in`, ensure you ran `autoreconf` first. Older seq66 sources may need that. ([GitHub](https://github.com/ahlstromcj/seq66/issues/25?utm_source=chatgpt.com "Configure fails · Issue #25 · ahlstromcj/seq66"))

## 5. Build Seq66

Compile the code:

```bash
make -j$(nproc)
```

`-j$(nproc)` speeds up compilation using all CPU cores. If you only want the command-line version, you can compile only that target after configure:

```bash
make seq66cli
```

## 6. Install

Once built, install it system-wide:

```bash
sudo make install
```

This typically installs binaries like **qseq66**, **qpseq66** (Qt GUI), and **seq66cli** into `/usr/local/bin` or similar.

## 7. Optional: Add to Desktop Menu

If the GUI version built (`qseq66` or `qpseq66`), a `.desktop` file already exists in the repo’s `desktop` folder. You can install it so it appears in your application launcher:

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
    

Ensure your MIDI devices are connected and recognized. Ubuntu Studio’s audio tools (like QJackCtl or Ubuntu Studio Audio Config) may help routing JACK/ALSA.

## Troubleshooting Tips

**Missing `configure` or autotools errors**

- Run `autoreconf --install --force` before `./configure`. ([GitHub](https://github.com/ahlstromcj/seq66/issues/25?utm_source=chatgpt.com "Configure fails · Issue #25 · ahlstromcj/seq66"))
    
- Ensure `automake`, `autoconf`, `libtool` are installed.
    

**Qt not detected**

- Confirm `qtbase5-dev`, `qttools5-dev`, `qttools5-dev-tools`, and `qmake` are installed.
    
- Use `qtchooser -list-versions` to check available Qt versions.
    

**Missing libs (such as liblo)**

- Install `liblo-dev` (for OSC) before configure. ([Reddit](https://www.reddit.com/r/sequencers/comments/16o7b7t/seq66_debian_install/?utm_source=chatgpt.com "Seq66 Debian install : r/sequencers"))

**Build fails due to ALSA/JACK**

- Check `libasound2-dev` and appropriate JACK dev packages are installed.

## Useful References

- Official Seq66 **README / INSTALL** in the repository (detailed build instructions). ([GitHub](https://github.com/ahlstromcj/seq66?utm_source=chatgpt.com "Seq66: Seq24-based live MIDI looper/editor. v. ..."))
    
- User manual and tutorials on the project site (great for usage after install). ([GitHub](https://raw.githubusercontent.com/ahlstromcj/seq66/master/data/share/doc/seq66-user-manual.pdf?utm_source=chatgpt.com "Seq66 User Manual v. 0.99.22 - GitHub"))
