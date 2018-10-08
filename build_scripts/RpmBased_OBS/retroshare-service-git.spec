Name:          retroshare-service-git
Version:       0.6.4
Release:       0
License:       GNU AFFERO GENERAL PUBLIC LICENSE version 3
Summary:       Secure chat and file sharing
Group:         Productivity/Networking/Other
Url:           http://retroshare.net
#Source0:       https://github.com/RetroShare/RetroShare/archive/v%{version}.tar.gz#/RetroShare-%{version}.tar.gz
Source0:       RetroShare-git.tar.gz
#Patch0:       various.patch
BuildRoot:     %{_tmppath}/%{name}
Conflicts:     retroshare-service
Requires:      libupnp openssl sqlcipher
BuildRequires: cmake git libupnp-devel openssl-devel sqlcipher-devel

%if %{defined centos_version}
BuildRequires: qt5-qtbase-devel qt5-qttools-devel qt5-qttools-static
Requires:      qt5-qtbase
%endif

%if 0%{?fedora_version}
BuildRequires: gcc-c++
BuildRequires: fdupes xapian-core-devel
BuildRequires: qt5-qtbase-devel qt5-qttools-devel qt5-qttools-static
Requires:      qt5-qtbase xapian-core
%endif

%if %{defined mageia}
BuildRequires: gcc-c++
BuildRequires: libzlib-devel libbzip2-devel
BuildRequires: libqt5core-devel libqt5xml-devel libxapian-devel
Requires:      libqt5core libqt5xml5 libxapian
%endif

%if 0%{?suse_version}
BuildRequires: gcc7 gcc7-c++
BuildRequires: fdupes libbz2-devel 
BuildRequires: libqt5-qtbase-devel libqt5-qttools-devel
BuildRequires: libxapian-devel update-desktop-files
Requires:      libbz2 libxapian
%endif

%if 0%{?fedora_version} >= 27
%undefine _debugsource_packages
%undefine _debuginfo_subpackages
%endif

%description
RetroShare is a cross-platform F2F communication platform.
It lets you share securely with your friends, using PGP
to authenticate peers and OpenSSL to encrypt all communication.
RetroShare provides filesharing, chat, messages and channels.
This package provides RetroShare system service that can be
controlled only via JSON API.

Authors:
see http://retroshare.net/
--------

%prep
%setup -n RetroShare-git
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
%endif

%if %{defined mageia}
QMAKE="qmake"
%endif

%if 0%{?suse_version}
BUILD_CC="QMAKE_CC=gcc-7"
BUILD_CXX="QMAKE_CXX=g++-7"
%endif

$QMAKE $BUILD_CC $BUILD_CXX QMAKE_STRIP=echo PREFIX="%{_prefix}" \
	BIN_DIR="%{_bindir}" \
	LIB_DIR="%{_libdir}" DATA_DIR="%{_datadir}/retroshare" CONFIG-=debug \
    CONFIG+=ipv6 CONFIG+=no_retroshare_android_service \
    CONFIG+=no_retroshare_android_notify_service \
    CONFIG+=no_retroshare_plugins CONFIG+=no_retroshare_nogui \
    CONFIG+=no_retroshare_gui CONFIG+=no_tests CONFIG+=no_libresapi \
    CONFIG+=no_libresapihttpserver CONFIG+=no_libresapilocalserver \
    CONFIG+=retroshare_service CONFIG+=rs_jsonapi ${BUILD_DEEPSEARCH} \
    CONFIG+=release RetroShare.pro
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
