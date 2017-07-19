Summary: Secure communication with friends
Name: retroshare
Version: 0.6.0.%{rev}
Release: 1%{?dist}
License: GPLv3
Group: Productivity/Networking/Other
URL: http://retroshare.sourceforge.net/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: qt5-qtbase-gui qt5-qtmultimedia qt5-qtx11extras qt5-qtbase
BuildRequires: gcc-c++ desktop-file-utils libgnome-keyring-devel glib2-devel libssh-devel protobuf-devel libcurl-devel libxml2-devel libxslt-devel openssl-devel libXScrnSaver-devel libupnp-devel bzip2-devel libmicrohttpd-devel qt5-qtx11extras-devel qt5-qttools-devel qt5-qtmultimedia-devel qt5-qttools-static
# This is because of sqlcipher:
BuildRequires: tcl

%description
RetroShare is a decentralized, private and secure commmunication and sharing platform. RetroShare provides filesharing, chat, messages, forums and channels.

Authors:
see http://retroshare.sourceforge.net/team.html

%package nogui
Summary:   RetroShare cli client
Group:     Productivity/Network/Other
Requires:  openssl
Conflicts: %name = %{version}

%description nogui
This is the command-line client for RetroShare network. This client can be contacted and talked-to using SSL. Clients exist for portable devices running e.g. Android.

%package voip-plugin
Summary:  RetroShare VOIP plugin
Group:    Productivity/Networking/Other
Requires: %name = %{version}
BuildRequires: opencv-devel speex-devel
%if 0%{?fedora} >= 22
BuildRequires: speexdsp-devel
%endif

%description voip-plugin
This package provides a plugin for RetroShare, a secured Friend-to-Friend communication platform. The plugin adds voice-over-IP functionality to the private chat window. Both friends chatting together need the plugin installed to be able to talk together.

%package feedreader-plugin
Summary:  RetroShare FeedReader plugin
Group:    Productivity/Networking/Other
Requires: %name = %{version}

%description feedreader-plugin
This package provides a plugin for RetroShare, a secured Friend-to-Friend communication platform. The plugin adds a RSS feed reader tab to retroshare.

%prep
%setup -q

%build
cd lib/sqlcipher
./configure --disable-shared --enable-static --enable-tempstore=yes CFLAGS="-fPIC -DSQLITE_HAS_CODEC" LDFLAGS="-lcrypto"
make
cd -
cd src
qmake-qt5 "CONFIG-=debug" "CONFIG+=release" PREFIX=%{_prefix} LIB_DIR=%{_libdir} RetroShare.pro
make
cd -

%install
rm -rf $RPM_BUILD_ROOT
cd src
make INSTALL_ROOT=$RPM_BUILD_ROOT install
#menu
desktop-file-validate $RPM_BUILD_ROOT%{_datadir}/applications/retroshare.desktop

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/retroshare
%defattr(644, root, root)
%{_datadir}/pixmaps/%{name}.xpm  
%{_datadir}/icons/hicolor
%{_datadir}/applications/%{name}.desktop
%{_datadir}/retroshare

%files nogui
%defattr(-, root, root)
%{_bindir}/retroshare-nogui
%defattr(644, root, root)
%{_datadir}/retroshare

%files voip-plugin
%defattr(-, root, root)
%{_libdir}/retroshare/extensions6/libVOIP.so*

%files feedreader-plugin
%defattr(-, root, root)
%{_libdir}/retroshare/extensions6/libFeedReader.so*

%changelog
* Sat Apr  4 2015 Heini <noreply@nowhere.net> - 
- Initial build.

