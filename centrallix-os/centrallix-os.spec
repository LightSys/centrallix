Summary: The core file-based ObjectSystem for the Centrallix server.
Name: centrallix-os
Version: 0.9.0
Release: 0
License: LGPL (/sys directory), BSD (/samples directory)
Group: System Environment/Libraries
Source: centrallix-os-%{version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
URL: http://www.centrallix.net/
Vendor: LightSys (http://www.lightsys.org)
BuildArch: noarch

%description
The centrallix-os package provides a core file-based ObjectSystem for
the Centrallix server.  This is analogous to the "web root" or
"document root" of a webserver.  The ObjectSystem contains a number of
important files that are needed for applications to run correctly;
they are roughly the "standard library" for Centrallix.  You can still
use Centrallix without the centrallix-os package, but you will not be
able to deploy Centrallix-generated DHTML web-based applications.

Centrallix need not run from a file-based ObjectSystem (any suitable
driver may be used for the ObjectSystem root), but it is normal to
use the file-based one.

%prep
%setup -q

%build
true

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/var/centrallix/os
mkdir -p $RPM_BUILD_ROOT/var/centrallix/os/samples
cp -r samples/* $RPM_BUILD_ROOT/var/centrallix/os/samples/
mkdir -p $RPM_BUILD_ROOT/var/centrallix/os/sys
cp -r sys/* $RPM_BUILD_ROOT/var/centrallix/os/sys/
mkdir -p $RPM_BUILD_ROOT/var/centrallix/os/tests
cp -r tests/* $RPM_BUILD_ROOT/var/centrallix/os/tests/
cp LICENSE.txt INSTALL.txt $RPM_BUILD_ROOT/var/centrallix/os/

%files
%defattr(-,root,root)
/var/centrallix/os/samples/*
/var/centrallix/os/sys/*
/var/centrallix/os/tests/*
/var/centrallix/os/LICENSE.txt
/var/centrallix/os/INSTALL.txt

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Tue Mar 25 2008 Greg Beeley <Greg.Beeley@LightSys.org> 0.9.0-0
- Following version 0.9.0 in the centrallix main package.

* Sat Feb 26 2005 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.4-0
- Initial creation of the RPM.
