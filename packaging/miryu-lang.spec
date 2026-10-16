Name:           miryu-lang
Version:        45.0.0
Release:        1%{?dist}
Summary:        A simple CLI to set the system locale on Linux/WSL

# Ensure _localedir is defined for rpmbuild environments that omit it.
%define _localedir %{_prefix}/share/locale

License:        GPLv2+
URL:            https://github.com/evernightvista/miryu-lang
Source0:        https://github.com/evernightvista/miryu-lang/archive/refs/heads/main.zip

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  gettext
BuildRequires:  gettext-devel
Requires:       glibc-all-langpacks
Requires:       systemd

%description
miryu-lang is a small command-line utility that lets the system administrator
switch the default system locale (LANG) on Linux — including WSL — with a
single, friendly command.Root privileges are required because the
locale is a system-wide setting.

%prep
%autosetup -n %{name}-main

%build
mkdir -p build
cd build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LOCALEDIR=%{_localedir} \
    ..
make %{?_smp_mflags}

%install
cd build
make install DESTDIR=%{buildroot}
cd ..
%find_lang %{name}

%files -f %{name}.lang
%license COPYING
%{_bindir}/%{name}

%changelog
* Fri Oct 16 2026 KairikiFedora <13278297951@sina.cn> - 45.0.0-1
- Initial release of miryu-lang.
- Supports zh-cn, zh-tw, ja, de, fr, ko.
- Includes gettext translations for zh_CN, zh_TW, ja, de, fr, ko.
