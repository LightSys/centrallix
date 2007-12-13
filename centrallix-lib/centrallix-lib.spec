Summary: A base functionality library developed for the Centrallix server.
Name: centrallix-lib
Version: 0.7.5
Release: 3
License: LGPL
Group: System Environment/Libraries
Source: centrallix-lib-%{version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
BuildPrereq: zlib-devel
URL: http://www.centrallix.net/
Vendor: LightSys (http://www.lightsys.org)

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
mv $RPM_BUILD_ROOT/usr/lib/libCentrallix.so $RPM_BUILD_ROOT/usr/lib/libCentrallix.so.%{version}
mv $RPM_BUILD_ROOT/usr/lib/libStParse.so $RPM_BUILD_ROOT/usr/lib/libStParse.so.%{version}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
/usr/lib/libCentrallix.so.%{version}
/usr/lib/libStParse.so.%{version}

%files devel
%defattr(-,root,root)
/usr/include/cxlib/*
/usr/lib/libCentrallix.a
/usr/lib/libStParse.a

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Thu Apr 20 2006 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.5-0
- Update to 0.7.5; many bug fixes.
- Added smmalloc module, strtcpy(), and the qpfPrintf() family of functions.

* Wed Sep  8 2004 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.4-0
- Initial creation of the RPM.
