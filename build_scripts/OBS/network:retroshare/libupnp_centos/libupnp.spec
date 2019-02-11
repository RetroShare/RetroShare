Version: 1.6.19
Summary: Universal Plug and Play (UPnP) SDK
Name: libupnp
Release: 2%{?dist}
License: BSD
Group: System Environment/Libraries
URL: http://www.libupnp.org/
Source: http://downloads.sourceforge.net/pupnp/%{name}-%{version}.tar.bz2

%define docdeveldir %{_docdir}/%{name}-devel-%{version}
%define docdir %{_docdir}/%{name}-%{version}

%description
The Universal Plug and Play (UPnP) SDK for Linux provides 
support for building UPnP-compliant control points, devices, 
and bridges on Linux.

%package devel
Group: Development/Libraries
Summary: Include files needed for development with libupnp
Requires: libupnp = %{version}-%{release}

%description devel
The libupnp-devel package contains the files necessary for development with
the UPnP SDK libraries.

%prep
%setup -q

%build
%configure --enable-static=no --enable-ipv6
make %{?_smp_mflags}

%install
test "$RPM_BUILD_ROOT" != "/" && rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%{__rm} %{buildroot}%{_libdir}/{libixml.la,libthreadutil.la,libupnp.la}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc LICENSE THANKS
%{_libdir}/libixml.so.2*
%{_libdir}/libthreadutil.so.6*
%{_libdir}/libupnp.so.6*

%files devel
%defattr(0644,root,root,0755)
#doc _devel_docs/*
%{_includedir}/upnp/
%{_libdir}/libixml.so
%{_libdir}/libthreadutil.so
%{_libdir}/libupnp.so
%{_libdir}/pkgconfig/libupnp.pc

%clean
rm -rf %{buildroot}

%changelog
* Sat Jun 07 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.6.19-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Mon Dec 09 2013 Adam Jackson <ajax@redhat.com> 1.6.19-1
- libupnp 1.6.19
- Build with --enable-ipv6 (#917210)

* Sun Oct 27 2013 Ville Skyttä <ville.skytta@iki.fi> - 1.6.18-4
- Adapt to possibly unversioned doc dirs.
- Include LICENSE and THANKS in main package.

* Sat Aug 03 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.6.18-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Tue Jan 29 2013 Adam Jackson <ajax@redhat.com> 1.6.18-1
- libupnp 1.6.18 (#905577)

* Tue Oct 16 2012 Adam Jackson <ajax@redhat.com> 1.6.17-1
- libupnp 1.6.17

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.6.13-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.6.13-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Sat Jul 30 2011 Matěj Cepl <mcepl@redhat.com> - 1.6.13-2
- Rebuilt against new libraries.

* Tue May 31 2011 Adam Jackson <ajax@redhat.com> 1.6.13-1
- libupnp 1.6.13

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.6.6-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Sat Jul 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.6.6-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.6.6-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Thu May 01 2008 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.6-1
- Update to version 1.6.6

* Sun Feb 03 2008 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.5-1
- Update to version 1.6.5

* Sun Jan 27 2008 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.4-1
- Update to version 1.6.4

* Fri Jan 04 2008 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.3-3
- No more building static library

* Sun Dec 30 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.3-2
- Spec file cleanup

* Sun Dec 30 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.3-1
- Update to version 1.6.3

* Thu Dec 13 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.2-1
- Update to version 1.6.2

* Sun Nov 18 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.1-1
- Update to version 1.6.1

* Wed Aug 29 2007 Fedora Release Engineering <rel-eng at fedoraproject dot org> - 1.6.0-2
- Rebuild for selinux ppc32 issue.

* Wed Jun 13 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.6.0-1
- Update to version 1.6.0

* Tue May 01 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.6-1
- Update to version 1.4.6

* Sat Apr 21 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.4-1
- Update to version 1.4.4

* Tue Mar 06 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.3-1
- Update to version 1.4.3

* Fri Feb 02 2007 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.2-1
- Update to version 1.4.2

* Wed Jul 05 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.1-1
- Update to version 1.4.1

* Fri Jun 23 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.0-3
- modified patch for x86_64 arch

* Fri Jun 23 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.0-2
- Add a patch for x86_64 arch

* Sun Jun 11 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.4.0-1
- Update to 1.4.0

* Sun Mar 05 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.3.1-1
- Update to 1.3.1

* Tue Feb 14 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.2.1a-6
- Rebuild for FC5

* Fri Feb 10 2006 Eric Tanguy <eric.tanguy@univ-nantes.fr> - 1.2.1a-5
- Rebuild for FC5

* Mon Jan  9 2006 Eric Tanguy 1.2.1a-4
- Include libupnp.so symlink in package to take care of non versioning of libupnp.so.1.2.1

* Sun Jan  8 2006 Paul Howarth 1.2.1a-3
- Disable stripping of object code for sane debuginfo generation
- Edit makefiles to hnnor RPM optflags
- Install libraries in %%{_libdir} rather than hardcoded /usr/lib
- Fix libupnp.so symlink
- Own directory %%{_includedir}/upnp
- Fix permissions in -devel package

* Fri Jan 06 2006 Eric Tanguy 1.2.1a-2
- Use 'install -p' to preserve timestamps
- Devel now require full version-release of main package

* Thu Dec 22 2005 Eric Tanguy 1.2.1a-1
- Modify spec file from 
http://rpm.pbone.net/index.php3/stat/4/idpl/2378737/com/libupnp-1.2.1a_DSM320-3.i386.rpm.html
