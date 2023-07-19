Name:    seq66
Version: 0.99.6
Release: 1%{?dist}
Summary: MIDI sequencer
License: GPL
URL:     https://github.com/ahlstromcj/seq66

Vendor:       Audinux
Distribution: Audinux

Source0: https://github.com/ahlstromcj/seq66/archive/refs/tags/%{version}.tar.gz#/%{name}-%{version}.tar.gz

BuildRequires: gcc gcc-c++ make
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: qt5-linguist
BuildRequires: qtchooser
BuildRequires: qt5-qttools
BuildRequires: git
BuildRequires: alsa-lib-devel
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtdeclarative-devel
BuildRequires: liblo-devel
BuildRequires: libglvnd-devel
BuildRequires: rtmidi-devel
BuildRequires: portmidi-devel
BuildRequires: jack-audio-connection-kit-devel
BuildRequires: desktop-file-utils

%description
Seq66 is a MIDI sequencer and live-looper with a hardware-sampler-like
grid-pattern interface, sets and playlists for song management, a scale
and chord-aware piano-roll interface, song editor for creative composition,
and control via MIDI automation for live performance.
Mute-groups enable/disable multiple patterns with one keystroke or
MIDI control.
Supports NSM (Non Session Manager) on Linux; can also run headless.
It does not support audio samples, just MIDI.

Seq66 is a major refactoring of Sequencer64/Kepler34, rebooting Seq24
with modern C++ and new features. Linux users can build this application
from the source code. See the INSTALL file; it has notes on many types on
installation. Windows users can get an installer package on GitHub or build
it with Qt Creator. Provides a comprehensive PDF user-manual.

%prep
%autosetup -n %{name}-%{version}

mkdir -p .local/bin
ln -s /usr/bin/qmake-qt5 .local/bin/qmake
ln -s /usr/bin/moc-qt5 .local/bin/moc
ln -s /usr/bin/uic-qt5 .local/bin/uic
ln -s /usr/bin/rcc-qt5 .local/bin/rcc
ln -s /usr/bin/lupdate-qt5 .local/bin/lupdate
ln -s /usr/bin/lrelease-qt5 .local/bin/lrelease

%build

%set_build_flags

%if 0%{?fedora} >= 38
export CXXFLAGS="-std=c++11 -include cstdint $CXXFLAGS"
%endif

export PATH=.local/bin:$PATH

./bootstrap

%configure --enable-cli
%make_build

%install

%make_install

install -m 755 -d %{buildroot}/%{_datadir}/icons/%{name}/
install -m 644 -p desktop/seq66.xpm %{buildroot}/%{_datadir}/icons/%{name}/
install -m 755 -d %{buildroot}/%{_datadir}/applications/
install -m 644 -p data/share/applications/seq66.desktop %{buildroot}%{_datadir}/applications/%{name}.desktop

desktop-file-install                         \
  --add-category="Audio;AudioVideo"	     \
  --delete-original                          \
  --dir=%{buildroot}%{_datadir}/applications \
    %{buildroot}/%{_datadir}/applications/%{name}.desktop

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/*.desktop

%files
%doc ChangeLog INSTALL NEWS README.md RELNOTES ROADMAP.md TODO
%{_bindir}/*
%{_datadir}/*
%{_includedir}/*
%{_libdir}/*

%changelog
* Sun Jul 02 2023 Yann Collette <ycollette.nospam@free.fr> - 0.99.6-1
- update 0.99.6-1

* Sat May 20 2023 Yann Collette <ycollette.nospam@free.fr> - 0.99.5-1
- update 0.99.5-1

* Sun Apr 30 2023 Yann Collette <ycollette.nospam@free.fr> - 0.99.4-1
- update 0.99.4-1

* Thu Apr 20 2023 Yann Collette <ycollette.nospam@free.fr> - 0.99.3-1
- update 0.99.3-1

* Mon Mar 20 2023 Yann Collette <ycollette.nospam@free.fr> - 0.99.2-1
- update 0.99.2-1

* Mon Jul 11 2022 Yann Collette <ycollette.nospam@free.fr> - 0.98.9.1-1
- initial version



