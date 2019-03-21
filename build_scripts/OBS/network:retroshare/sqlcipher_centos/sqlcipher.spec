#
# spec file for package 
#
# Copyright (c) 2013 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

Name:           sqlcipher
Version:        3.3.1
Release:	0
License:	BSD-3-Clause
Summary:	SQLite database encryption
Url:		http://sqlcipher.net
Group:		Productivity/Databases/Clients
Source:		%name-%{version}.tar.bz2
BuildRequires:	pkgconfig
BuildRequires:	tcl-devel
BuildRequires:	pkgconfig(openssl)
BuildRequires:	readline-devel
BuildRequires:	pkgconfig(sqlite3)
#Requires:	libncurses5
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
SQLCipher is an SQLite extension that provides transparent 256-bit AES encryption of database files. Pages are encrypted before being written to disk and are decrypted when read back. Due to the small footprint and great performance it’s ideal for protecting embedded application databases and is well suited for mobile development.

%package -n lib%{name}-3_8_10_2-0
Group:		System/Libraries
Summary:	Shared library for SQLCipher

%description -n lib%{name}-3_8_10_2-0
SQLCipher library.

SQLCipher is an SQLite extension that provides transparent 256-bit AES encryption of database files. Pages are encrypted before being written to disk and are decrypted when read back. Due to the small footprint and great performance it’s ideal for protecting embedded application databases and is well suited for mobile development.

%package devel
Group:		Development/Libraries/C and C++
Summary:	Development files for SQLCipher
#Requires:	sqlite3-devel
#Requires:	libopenssl-devel

%description devel
Development files for SQLCipher.

SQLCipher is an SQLite extension that provides transparent 256-bit AES encryption of database files. Pages are encrypted before being written to disk and are decrypted when read back. Due to the small footprint and great performance it’s ideal for protecting embedded application databases and is well suited for mobile development.


%prep
%setup -q

%build
%configure --enable-threadsafe --enable-cross-thread-connections --enable-releasemode --with-crypto-lib --disable-tcl --enable-tempstore=yes CFLAGS="-fPIC -DSQLITE_HAS_CODEC" LDFLAGS="-lcrypto"
make %{?_smp_mflags}

%install
%make_install

%post -n lib%{name}-3_8_10_2-0
/sbin/ldconfig

%postun -n lib%{name}-3_8_10_2-0
/sbin/ldconfig

%files
%defattr(-,root,root)
%doc README.md LICENSE
%_bindir/sqlcipher

%files -n lib%{name}-3_8_10_2-0
%defattr(-,root,root)
%doc README.md LICENSE
%_libdir/libsqlcipher-3.8.10.2.so.*

%files devel
%defattr(-,root,root)
%doc README.md LICENSE
%_libdir/libsqlcipher.a
%_libdir/libsqlcipher.la
%_libdir/libsqlcipher.so
%_libdir/pkgconfig/sqlcipher.pc
#%_libdir/tcl/tcl*/sqlite3
%_includedir/sqlcipher

%changelog

