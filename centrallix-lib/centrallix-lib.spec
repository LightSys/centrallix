Summary: A base functionality library developed for the Centrallix server.
Name: centrallix-lib
Version: 0.9.1
Release: 0
License: LGPL
Group: System Environment/Libraries
Source: centrallix-lib-%{version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
BuildPrereq: zlib-devel
URL: http://www.centrallix.net/
Vendor: LightSys (http://www.lightsys.org)
%define librelease 0

%description
The centrallix-lib package provides required base functionality for the
Centrallix application platform.  It is also used by other applications
as well.

%package devel
Summary: Development libraries and includes for centrallix-lib
Group: Development/Libraries
Requires: centrallix-lib = %{version}, zlib-devel

%description devel
The centrallix-lib-devel package is necessary to build applications
which use the centrallix-lib library.  It is not needed to run 
applications which have already been linked against centrallix-lib.

%prep
%setup -q

%build
./configure --prefix=$RPM_BUILD_ROOT/usr --enable-optimization --with-hardening=low
make

%install
rm -rf $RPM_BUILD_ROOT
make install
#mv $RPM_BUILD_ROOT/usr/lib/libCentrallix.so.%{version}.%{release} $RPM_BUILD_ROOT/usr/lib/libCentrallix.so.%{version}.%{release}
#mv $RPM_BUILD_ROOT/usr/lib/libStParse.so.%{version}.%{release} $RPM_BUILD_ROOT/usr/lib/libStParse.so.%{version}.%{release}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
/usr/lib/libCentrallix.so.%{version}.%{librelease}
/usr/lib/libStParse.so.%{version}.%{librelease}
/usr/lib/libCentrallix.so.%{version}
/usr/lib/libStParse.so.%{version}

%files devel
%defattr(-,root,root)
/usr/include/cxlib/*
/usr/lib/libCentrallix.a
/usr/lib/libCentrallix.so
/usr/lib/libStParse.a
/usr/lib/libStParse.so

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Mon Sep 13 2010 Greg Beeley <Greg.Beeley@LightSys.org> 0.9.1-0
- Update to 0.9.1
- Many bug fixes, including a major rewrite of the 'mtlexer'
  module for robustness and security reasons.
- Disabled call graph profiler (-pg) by default.

* Thu Mar 22 2008 Greg Beeley <Greg.Beeley@LightSys.org> 0.9.0-0
- Update to 0.9.0 to sync with main centrallix distribution.
- Many bug fixes.

* Thu Apr 20 2006 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.5-0
- Update to 0.7.5; many bug fixes.
- Added smmalloc module, strtcpy(), and the qpfPrintf() family of functions.

* Wed Sep  8 2004 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.4-0
- Initial creation of the RPM.
