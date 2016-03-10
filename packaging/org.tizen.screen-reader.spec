Name:       org.tizen.screen-reader
Summary:    Screen Reader Assistive Technology
Version:    0.0.8
Release:    1
License:    Flora-1.1
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  at-spi2-core
BuildRequires:  at-spi2-core-devel
BuildRequires:  cmake
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(ecore)
%if %{with x}
BuildRequires:  pkgconfig(ecore-x)
%endif
%if %{with wayland}
BuildRequires:  pkgconfig(ecore-wayland)
%endif
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
%if "%{?profile}" != "tv"
BuildRequires:  pkgconfig(tapi)
%endif
BuildRequires:  pkgconfig(libtzplatform-config)

%description
An utility library for developers of the menu screen.

%prep
%setup -q

%build
%define DataDir %{?TZ_SYS_RO_SHARE:%TZ_SYS_RO_SHARE}%{!?TZ_SYS_RO_SHARE:/usr/share}
%define AppDir %{TZ_SYS_RO_APP}/%{name}
%define Exec screen-reader
%bcond_with x
%bcond_with wayland

rm -rf CMakeFiles CMakeCache.txt

%if "%{profile}" != "tv"
        export SEC_FEATURE_TAPI_ENABLE="1"
        export CFLAGS+=" -DELM_ACCESS_KEYBOARD"
%else
        export SEC_FEATURE_TAPI_ENABLE="0"
%endif

cmake . -DCMAKE_INSTALL_PREFIX="%{AppDir}" \
        -DCMAKE_TARGET="%{Exec}" \
        -DCMAKE_PACKAGE="%{name}" \
        -DTZ_SYS_RO_APP=%{TZ_SYS_RO_APP} \
        -DTZ_SYS_RO_PACKAGES=%{TZ_SYS_RO_PACKAGES} \
%if %{with x}
        -DX11_ENABLED=1 \
%endif
        -DSEC_FEATURE_TAPI_ENABLE=${SEC_FEATURE_TAPI_ENABLE}

make %{?jobs:-j%jobs} \
2>&1 | sed \
-e 's%^.*: error: .*$%\x1b[37;41m&\x1b[m%' \
-e 's%^.*: warning: .*$%\x1b[30;43m&\x1b[m%'
export LD_LIBRARY_PATH=/emul/ia32-linux/lib:/emul/ia32-linux/usr/lib:$LD_LIBRARY_PATH
export CTEST_OUTPUT_ON_FAILURE=1
make test

%install
rm -rf %{buildroot}
%make_install

%post
/sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest org.tizen.screen-reader.manifest
%{AppDir}/bin/screen-reader
%{AppDir}/res/icons/screen-reader.png
%{AppDir}/res/locale/*/LC_MESSAGES/*
%{DataDir}/packages/%{name}.xml
