Summary: Secure communication with friends
Name: retroshare06
Version: 0.6.0.%{rev}
Release: 1%{?dist}
License: GPLv3
Group: Productivity/Networking/Other
URL: http://retroshare.sourceforge.net/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: qt-x11
BuildRequires: gcc-c++ qt-devel desktop-file-utils libgnome-keyring-devel glib2-devel libssh-devel protobuf-devel libcurl-devel libxml2-devel libxslt-devel openssl-devel libXScrnSaver-devel libupnp-devel bzip2-devel libmicrohttpd-devel
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
%setup -q -a 0

%build
cd lib/sqlcipher
./configure --disable-shared --enable-static --enable-tempstore=yes CFLAGS="-DSQLITE_HAS_CODEC" LDFLAGS="-lcrypto"
make
cd -
cd src
qmake-qt4 CONFIG=release RetroShare.pro
make
cd -

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_datadir}/pixmaps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/24x24/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/64x64/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/RetroShare06
mkdir -p $RPM_BUILD_ROOT/usr/lib/retroshare/extensions6
#bin
install -m 755 src/retroshare-gui/src/RetroShare $RPM_BUILD_ROOT%{_bindir}/RetroShare06
install -m 755 src/retroshare-nogui/src/retroshare-nogui $RPM_BUILD_ROOT%{_bindir}/RetroShare06-nogui
#icons
install -m 644 src/data/retroshare.xpm $RPM_BUILD_ROOT%{_datadir}/pixmaps/retroshare06.xpm
install -m 644 src/data/24x24/retroshare.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/24x24/apps/retroshare06.png
install -m 644 src/data/48x48/retroshare.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/retroshare06.png
install -m 644 src/data/64x64/retroshare.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/64x64/apps/retroshare06.png
install -m 644 src/data/retroshare.desktop $RPM_BUILD_ROOT%{_datadir}/applications/retroshare06.desktop
install -m 644 src/libbitdht/src/bitdht/bdboot.txt $RPM_BUILD_ROOT%{_datadir}/RetroShare06/
#plugins
install -m 755 src/plugins/VOIP/libVOIP.so $RPM_BUILD_ROOT/usr/lib/retroshare/extensions6/
install -m 755 src/plugins/FeedReader/libFeedReader.so $RPM_BUILD_ROOT/usr/lib/retroshare/extensions6/
#menu
desktop-file-validate $RPM_BUILD_ROOT%{_datadir}/applications/retroshare06.desktop

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/RetroShare06
%defattr(644, root, root)
%{_datadir}/pixmaps/%{name}.xpm  
%{_datadir}/icons/hicolor
%{_datadir}/applications/%{name}.desktop
%{_datadir}/RetroShare06

%files nogui
%defattr(-, root, root)
%{_bindir}/RetroShare06-nogui
%defattr(644, root, root)
%{_datadir}/RetroShare06

%files voip-plugin
%defattr(-, root, root)
/usr/lib/retroshare/extensions6/libVOIP.so

%files feedreader-plugin
%defattr(-, root, root)
/usr/lib/retroshare/extensions6/libFeedReader.so

%changelog
* Sat Apr  4 2015 Heini <noreply@nowhere.net> - 
- Initial build.

