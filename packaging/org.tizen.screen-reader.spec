%define AppInstallPath /usr/apps/%{name}
%define Exec screen-reader


Name:       org.tizen.screen-reader
Summary:    Screen Reader Assistive Technology
Version:    0.0.6
Release:    1
License:    Flora-1.1
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  at-spi2-core
BuildRequires:  at-spi2-core-devel
BuildRequires:  cmake
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(eldbus)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(capi-media-tone-player)
BuildRequires:  pkgconfig(capi-system-device)
BuildRequires:  tts
BuildRequires:  tts-devel
BuildRequires:  vconf
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(check)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(capi-network-wifi)
%if "%{?tizen_profile_name}" != "tv"
BuildRequires:  pkgconfig(tapi)
%endif

%description
An utility library for developers of the menu screen.

%prep
%setup -q

%build
rm -rf CMakeFiles CMakeCache.txt

%if "%{?tizen_profile_name}" != "tv"
        export SEC_FEATURE_TAPI_ENABLE="1"
%else
        export SEC_FEATURE_TAPI_ENABLE="0"
%endif
export CFLAGS+=" -DELM_ACCESS_KEYBOARD"

cmake . -DCMAKE_INSTALL_PREFIX="%{AppInstallPath}" \
        -DCMAKE_TARGET="%{Exec}" \
        -DCMAKE_PACKAGE="%{name}" \
        -DSEC_FEATURE_TAPI_ENABLE=${SEC_FEATURE_TAPI_ENABLE}

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

%postun -p /sbin/ldconfig

%files
%manifest org.tizen.screen-reader.manifest
%{AppInstallPath}/bin/screen-reader
%{AppInstallPath}/res/icons/screen-reader.png
%{AppInstallPath}/res/locale/*/LC_MESSAGES/*
/opt/share/packages/%{name}.xml
