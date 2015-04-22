Name:       org.tizen.screen-reader
Summary:    Empty app
Version:    0.0.1
Release:    1
Group:      Applications/Core Applications
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  at-spi2-core
BuildRequires:  at-spi2-core-devel
BuildRequires:  cmake
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(capi-system-device)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(bundle)
BuildRequires:  tts
BuildRequires:  tts-devel
BuildRequires:  vconf
BuildRequires:  pkgconfig(check)

%define AppInstallPath /usr/apps/%{name}
%define Exec screen-reader


%description
An utility library for developers of the menu screen.

%prep
%setup -q

%build
cmake . -DCMAKE_INSTALL_PREFIX="%{AppInstallPath}" -DCMAKE_TARGET="%{Exec}" -DCMAKE_PACKAGE="%{name}"
make %{?jobs:-j%jobs} \
2>&1 | sed \
-e 's%^.*: error: .*$%\x1b[37;41m&\x1b[m%' \
-e 's%^.*: warning: .*$%\x1b[30;43m&\x1b[m%'
export LD_LIBRARY_PATH=/emul/ia32-linux/lib:/emul/ia32-linux/usr/lib:$LD_LIBRARY_PATH
make test

%install
rm -rf %{buildroot}
%make_install

%post 
/sbin/ldconfig
vconftool set -t string db/setting/accessibility/language "en_US"
vconftool set -t int db/setting/accessibility/information_level 2
vconftool set -t int db/setting/accessibility/voice 1
vconftool set -t string db/setting/accessibility/tracking_signal "focused"

%postun -p /sbin/ldconfig

%files
%manifest org.tizen.screen-reader.manifest
%{AppInstallPath}/bin/screen-reader
%{AppInstallPath}/res/icons/screen-reader.png
/usr/share/packages/%{name}.xml
