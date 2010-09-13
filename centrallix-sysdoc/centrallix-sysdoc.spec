Summary: Internal documentation, API's, and specifications for Centrallix
Name: centrallix-sysdoc
Version: 0.9.1
Release: 0
License: BSD
Group: System Environment/Libraries
Source: centrallix-sysdoc-%{version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
URL: http://www.centrallix.net/
Vendor: LightSys (http://www.lightsys.org)
BuildArch: noarch

%description
This package provides internal developer documentation for Centrallix.  If
you want to help develop the Centrallix project, or desire to find out more
about how Centrallix works internally, this is a good place to start.

%prep
%setup -q

%build
true

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/SystemDocs
cp -r * $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/SystemDocs/
rm $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/SystemDocs/centrallix-sysdoc.spec
rm $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/SystemDocs/mkrpm

%files
%defattr(-,root,root)
/usr/share/doc/centrallix-%{version}/SystemDocs

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Mon Sep 13 2010 Greg Beeley <Greg.Beeley@LightSys.org> 0.9.1-0
- Sync with 0.9.1 release of main code

* Thu Mar 27 2008 Greg Beeley <Greg.Beeley@LightSys.org> 0.9.0-0
- Sync with 0.9.0 release of main code

* Tue Mar 01 2005 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.4-0
- Initial creation of the RPM.
