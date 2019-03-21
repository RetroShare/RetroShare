Name:          retroshare
Version:       0.6.4
Release:       0
License:       GNU AFFERO GENERAL PUBLIC LICENSE version 3
Summary:       Secure chat and file sharing
Group:         Productivity/Networking/Other
Url:           http://retroshare.net
Source0:       https://github.com/RetroShare/RetroShare/archive/v%{version}.tar.gz#/RetroShare-%{version}.tar.gz
#Patch0:       various.patch
BuildRoot:     %{_tmppath}/%{name}
BuildRequires: gcc-c++
BuildRequires: desktop-file-utils
BuildRequires: glib2-devel sqlcipher-devel libmicrohttpd-devel
BuildRequires: openssl-devel

%if 0%{?centos_version} == 600
BuildRequires: gnome-keyring-devel
%else
BuildRequires: libgnome-keyring-devel
%endif
%if 0%{?centos_version} >= 800 || 0%{?suse_version} || 0%{?fedora_version}
BuildRequires: fdupes
%if 0%{?suse_version}
BuildRequires: libqt5-qtbase-devel
BuildRequires: libqt5-qtx11extras-devel
BuildRequires: libqt5-qtmultimedia-devel
BuildRequires: libqt5-qttools-devel
%else
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtx11extras-devel
BuildRequires: qt5-qtmultimedia-devel
BuildRequires: qt5-qttools-devel
BuildRequires: qt5-qttools-static
%endif
%else
BuildRequires: libqt4-devel
%endif

%if %{defined mageia}
BuildRequires: libxscrnsaver-devel
%else
BuildRequires: libXScrnSaver-devel
%endif

%if 0%{?suse_version}
BuildRequires: update-desktop-files libbz2-devel
%endif

BuildRequires: libupnp-devel
Requires:      openssl %name-nogui = %{version}
Conflicts:     retroshare-git

%if 0%{?fedora_version} >= 27
%undefine _debugsource_packages
%undefine _debuginfo_subpackages
%endif

%description
RetroShare is a cross-platform F2F communication platform.
It lets you share securely with your friends, using PGP
to authenticate peers and OpenSSL to encrypt all communication.
RetroShare provides filesharing, chat, messages and channels.

Authors:
see http://retroshare.net/
--------

%package nogui
Summary:  RetroShare without gui
Group:    Productivity/Network/Other
Requires: openssl

%description nogui
retroshare-nogui comes without a user interface. It can be controlled via a webui.

%prep
%setup -n RetroShare-%{version}
#%patch0 -p0

%build
%if 0%{?centos_version} >= 800 || 0%{?suse_version} || 0%{?fedora_version}
QMAKE="qmake-qt5"
%else
QMAKE="qmake-qt4"
%endif

$QMAKE CONFIG-=debug CONFIG+=release CONFIG+=no_retroshare_plugins QMAKE_STRIP=echo PREFIX="%{_prefix}" BIN_DIR="%{_bindir}" LIB_DIR="%{_libdir}" DATA_DIR="%{_datadir}/retroshare" RetroShare.pro
make

%install
rm -rf $RPM_BUILD_ROOT
make INSTALL_ROOT=$RPM_BUILD_ROOT install

%if 0%{?centos_version} < 800
%else
%fdupes %{buildroot}/%{_prefix}
%endif

#menu
%if 0%{?suse_version}
%suse_update_desktop_file -n -i retroshare Network P2P
%endif
%if 0%{?fedora_version}
desktop-file-validate $RPM_BUILD_ROOT%{_datadir}/applications/retroshare.desktop
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%{_bindir}/retroshare
%defattr(644, root, root)
%{_datadir}/pixmaps/retroshare.xpm
%{_datadir}/icons/hicolor/
%{_datadir}/applications/retroshare.desktop

%files nogui
%defattr(-, root, root)
%{_bindir}/retroshare-nogui
%defattr(644, root, root)
%{_datadir}/retroshare

%changelog
