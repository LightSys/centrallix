Summary: Internal documentation, API's, and specifications for Centrallix
Name: centrallix-sysdoc
Version: 0.7.4
Release: 2
License: BSD
Group: System Environment/Libraries
Source: centrallix-sysdoc-%{version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
URL: http://www.centrallix.net/
Vendor: LightSys (http://www.lightsys.org)

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
rm -r $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/SystemDocs/CVS
rm $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/SystemDocs/centrallix-sysdoc.spec
rm $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/SystemDocs/mkrpm

%files
%defattr(-,root,root)
/usr/share/doc/centrallix-%{version}/SystemDocs

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Tue Mar 01 2005 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.4-0
- Initial creation of the RPM.
