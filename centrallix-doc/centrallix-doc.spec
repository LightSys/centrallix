Summary: User, System admin, and Application developer docs for Centrallix
Name: centrallix-doc
Version: 0.9.0
Release: 0
License: BSD
Group: System Environment/Libraries
Source: centrallix-doc-%{version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
URL: http://www.centrallix.net/
Vendor: LightSys (http://www.lightsys.org)
BuildArch: noarch

%description
This package provides documentation for Centrallix that may be useful to
an application developer or system manager of a Centrallix installation.
For internal documentation and specifications, see centrallix-sysdoc.

%prep
%setup -q

%build
true

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/UserDocs
cp -r * $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/UserDocs/
rm $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/UserDocs/centrallix-doc.spec
rm $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}/UserDocs/mkrpm

%files
%defattr(-,root,root)
/usr/share/doc/centrallix-%{version}/UserDocs

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Thu Mar 27 2008 Greg Beeley <Greg.Beeley@LightSys.org> 0.9.0-0
- Sync with 0.9.0 release of main code

* Tue Mar 01 2005 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.4-0
- Initial creation of the RPM.
