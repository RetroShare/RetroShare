Name:          retroshare-service
Version:       0.6.5
Release:       0
License:       AGPL-3.0-or-later
Summary:       Secure distributed chat, mail, forums, file sharing etc
Group:         Productivity/Networking/Other
Url:           https://retroshare.cc
Source0:       RetroShare.tar.gz
#Patch0:       various.patch
BuildRoot:     %{_tmppath}/%{name}
Conflicts:     retroshare-service-unstable
BuildRequires: cmake doxygen git openssl-devel sqlcipher-devel

%if %{defined centos_version}
BuildRequires: devtoolset-7-toolchain devtoolset-7-libstdc++-devel
BuildRequires: qt5-qtbase-devel qt5-qttools-devel qt5-qttools-static
BuildRequires: libupnp-devel
%endif

%if 0%{?fedora_version}
BuildRequires: gcc-c++
BuildRequires: fdupes xapian-core-devel
BuildRequires: qt5-qtbase-devel qt5-qttools-devel qt5-qttools-static
%endif

%if 0%{?fedora_version} >= 27
%undefine _debugsource_packages
%undefine _debuginfo_subpackages
%endif

%if 0%{?fedora_version} >= 29
BuildRequires: miniupnpc-devel
%else
BuildRequires: libupnp-devel
%endif

%if %{defined mageia}
BuildRequires: gcc-c++
BuildRequires: libzlib-devel libbzip2-devel
BuildRequires: libqt5core-devel libqt5xml-devel libxapian-devel
BuildRequires: libupnp-devel
%endif

%if 0%{?suse_version}
BuildRequires: gcc7 gcc7-c++
BuildRequires: fdupes libbz2-devel 
BuildRequires: libqt5-qtbase-devel libqt5-qttools-devel
BuildRequires: libxapian-devel update-desktop-files
BuildRequires: libminiupnpc-devel
%endif

%description
RetroShare is a cross-platform F2F communication platform.
It lets you share securely with your friends, using PGP
to authenticate peers and OpenSSL to encrypt all communication.
RetroShare provides filesharing, chat, messages and channels.
This package provides RetroShare system service that can be
controlled only via JSON API.

Authors:
see https://retroshare.cc/
--------

%prep
%setup -n RetroShare
#%patch0 -p0

%build

nproc
qmake --version || qmake-qt5 --version
ls $(which gcc)*
ls $(which g++)*

BUILD_CC=""
BUILD_CXX=""
BUILD_DEEPSEARCH="CONFIG+=rs_deep_search"
QMAKE="qmake-qt5"

%if %{defined centos_version}
# Xapian is not availabe on Centos 7
BUILD_DEEPSEARCH="CONFIG+=no_rs_deep_search"
source /opt/rh/devtoolset-7/enable
%endif

%if %{defined mageia}
QMAKE="qmake"
%endif

%if 0%{?suse_version}
BUILD_CC="QMAKE_CC=gcc-7"
BUILD_CXX="QMAKE_CXX=g++-7"
RS_UPNP_LIB="RS_UPNP_LIB=miniupnpc"
echo suse_version: %{?suse_version}
%endif

# On OpenSuse Tumbleweed and Factory ARM 64 bits seems cmake/restbed and
# qmake/retroshare have PIE/PIC mismatch attempt to coherce restbed to be PIC
# https://en.opensuse.org/Archive:How_to_detect_Tumbleweed
%if 0%{?suse_version} == 1550 && ( (%{_arch} == "x86_64") || (%{_arch} == "aarch64") )
RESTBED_CMAKE_FILE="supportlibs/restbed/CMakeLists.txt"
echo 'set(CMAKE_POSITION_INDEPENDENT_CODE ON)' | \
	cat - $RESTBED_CMAKE_FILE > tmp_cmake && mv tmp_cmake $RESTBED_CMAKE_FILE
cat $RESTBED_CMAKE_FILE
%endif

%if 0%{?fedora_version} >= 29
RS_UPNP_LIB="RS_UPNP_LIB=miniupnpc"
%endif

$QMAKE $BUILD_CC $BUILD_CXX QMAKE_STRIP=echo PREFIX="%{_prefix}" \
	BIN_DIR="%{_bindir}" ${RS_UPNP_LIB} \
	LIB_DIR="%{_libdir}" DATA_DIR="%{_datadir}/retroshare" CONFIG-=debug \
    CONFIG+=ipv6 CONFIG+=no_retroshare_android_service \
    CONFIG+=no_retroshare_android_notify_service \
    CONFIG+=no_retroshare_plugins CONFIG+=no_retroshare_nogui \
    CONFIG+=no_retroshare_gui CONFIG+=no_tests CONFIG+=no_libresapi \
    CONFIG+=no_libresapihttpserver CONFIG+=no_libresapilocalserver \
    CONFIG+=retroshare_service CONFIG+=rs_jsonapi ${BUILD_DEEPSEARCH} \
    CONFIG+=release \
    RS_MAJOR_VERSION=0 RS_MINOR_VERSION=6 RS_MINI_VERSION=5 \
    RS_EXTRA_VERSION="-retroshare-service-OBS-RPM" \
    RetroShare.pro
make -j$(nproc) || make -j$(nproc) || make

%install
rm -rf $RPM_BUILD_ROOT
make INSTALL_ROOT=$RPM_BUILD_ROOT install

%if 0%{?centos_version} < 800
%else
%fdupes %{buildroot}/%{_prefix}
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%{_bindir}/retroshare-service
%defattr(644, root, root)
%{_datadir}/retroshare

%changelog
