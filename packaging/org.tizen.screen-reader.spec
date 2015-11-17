%define AppInstallPath /usr/apps/%{name}
%define Exec screen-reader
%bcond_with x
%bcond_with wayland

Name:       org.tizen.screen-reader
Summary:    Screen Reader Assistive Technology
Version:    0.0.7
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
BuildRequires:  pkgconfig(lua)

%description
An utility library for developers of the menu screen.

%prep
%setup -q

%build
rm -rf CMakeFiles CMakeCache.txt

%if "%{profile}" != "tv"
        export SEC_FEATURE_TAPI_ENABLE="1"
        export CFLAGS+=" -DELM_ACCESS_KEYBOARD"
%else
        export SEC_FEATURE_TAPI_ENABLE="0"
%endif

cmake . -DCMAKE_INSTALL_PREFIX="%{AppInstallPath}" \
        -DCMAKE_TARGET="%{Exec}" \
        -DCMAKE_PACKAGE="%{name}" \
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
%{AppInstallPath}/bin/screen-reader
%{AppInstallPath}/res/icons/screen-reader.png
%{AppInstallPath}/res/locale/*/LC_MESSAGES/*
/usr/share/packages/%{name}.xml
%{AppInstallPath}/res/scripts/*
