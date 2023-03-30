Summary: The Centrallix application platform server.
Name: centrallix
Version: 0.9.1
Release: 0
License: GPL
Group: System Environment/Daemons
Source: centrallix-%{version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
BuildPrereq: zlib-devel, centrallix-lib-devel = %{version}
Requires: zlib, centrallix-lib = %{version}, centrallix-os = %{version}
URL: http://www.centrallix.net/
Vendor: LightSys (http://www.lightsys.org/)

%description
The Centrallix package provides the Centrallix application platform
server, an innovative new software development paradigm focused on
object and rule based declarative development, and featuring rich
web-based application deployment with zero client footprint.

%package sybase
Summary: The Sybase ASE ObjectSystem Driver
Group: System Environment/Daemons
Requires: centrallix = %{version}

%description sybase
The Sybase ASE ObjectSystem Driver provides Centrallix access to the
Sybase ASE database server via the Open Client / TDS protocol and
ct-library interface.  This is provided as a separate package since
not everyone has sybase-ocsd installed.

%package xml
Summary: The XML ObjectSystem Driver
Group: System Environment/Daemons
Requires: centrallix = %{version}

%description xml
The XML ObjectSystem Driver provides Centrallix access to XML files
and data wherever it may occur.  This is provided as a separate
package since it has dependencies on particular versions of the
libxml library.  This way, more users can make use of the prebuilt
RPM files (albeit without XML support if their XML version does
not match). Note that versions of the library prior to 2.9.13 have 
trouble parsing UTF-16 text when characters are split by read boundaries. 
See this issue for a detailed look at the bug: 
https://gitlab.gnome.org/GNOME/libxml2/-/issues/458

%package mysql
Summary: The MySQL ObjectSystem Driver
Group: System Environment/Daemons
Requires: centrallix = %{version}

%description mysql
The MySQL ObjectSystem Driver provides Centrallix access to the
MySQL database server.  This is provided as a separate package so
that the MySQL libraries need not be installed to build and run
Centrallix.

%prep
%setup -q

%build
./configure --prefix=$RPM_BUILD_ROOT/usr --disable-optimization --enable-sybase --enable-mysql --with-logmethod=syslog --with-centrallix-inc=/usr/include --sysconfdir=$RPM_BUILD_ROOT/etc --localstatedir=$RPM_BUILD_ROOT/var --with-builddir=$RPM_BUILD_ROOT
make
make test_obj
make test_prt
make modules
make config

%pre
echo "**** Centrallix Application Platform Installation **** PLEASE READ ****"
echo ""
echo "You are installing the Centrallix application platform server.  This"
echo "program is free software; you can redistribute it and/or modify it"
echo "under the terms of the GNU General Public License as published by the"
echo "Free Software Foundation; either version 2 of the License, or (at"
echo "your option) any later version."
echo ""
echo "This program is distributed in the hope that it will be useful, but"
echo "WITHOUT ANY WARRANTY; without even the implied warranty of"
echo "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU"
echo "General Public License for more details."
echo ""
echo "For details on copying, warranty, modification, and redistribution,"
echo "see the files 'COPYING' and 'LICENSE' that accompany this distribution."
echo ""
echo "This version is a DEVELOPMENT release, and certainly has the associated"
echo "higher level of bugs and of incompleteness that accompany software"
echo "under heavy development.  We recommend firewalling this software from"
echo "access by untrusted users."
echo ""
exit 0

%post
[ -f /sbin/chkconfig -a "$1" = "1" ] && /sbin/chkconfig --add centrallix
exit 0

%preun
[ -f /sbin/chkconfig -a "$1" = "0" ] && /sbin/chkconfig --del centrallix
exit 0

%install
rm -rf $RPM_BUILD_ROOT
make install
make modules_install
make config_install
make rhinit_install
make test_install
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}
cp COPYING AUTHORS RELNOTES.txt README LICENSE CHANGELOG $RPM_BUILD_ROOT/usr/share/doc/centrallix-%{version}
mkdir -p $RPM_BUILD_ROOT/var/centrallix/os
echo "<html><head><title>Centrallix</title></head><body>Centrallix" %{version} "Successfully Installed.<br><br>Please install a centrallix-os package for full functionality!</body></html>" > $RPM_BUILD_ROOT/var/centrallix/os/index.html

%files
%defattr(-,root,root)
/usr/sbin/centrallix
/var/centrallix/os/index.html
/usr/bin/test_obj
/usr/bin/test_prt
/usr/lib/centrallix/objdrv_http.so
/usr/lib/centrallix/objdrv_mime.so
/usr/lib/centrallix/objdrv_gzip.so
/usr/lib/centrallix/objdrv_mbox.so
/usr/lib/centrallix/objdrv_pop3_v3.so
/usr/share/doc/centrallix-%{version}/COPYING
/usr/share/doc/centrallix-%{version}/AUTHORS
/usr/share/doc/centrallix-%{version}/RELNOTES.txt
/usr/share/doc/centrallix-%{version}/README
/usr/share/doc/centrallix-%{version}/LICENSE
/usr/share/doc/centrallix-%{version}/CHANGELOG
%verify(not md5 size mtime) %config(noreplace) /etc/centrallix.conf
%verify(not md5 size mtime) %config(noreplace) /etc/centrallix/rootnode
%config /etc/centrallix/useragent.cfg
%config /etc/centrallix/types.cfg
/etc/rc.d/init.d/centrallix

%files sybase
%defattr(-,root,root)
/usr/lib/centrallix/objdrv_sybase.so

%files xml
%defattr(-,root,root)
/usr/lib/centrallix/objdrv_xml.so

%files mysql
%defattr(-,root,root)
/usr/lib/centrallix/objdrv_mysql.so

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Mon Sep 13 2010 Greg Beeley <Greg.Beeley@LightSys.org> 0.9.1-0
- major development release; see changelog and release notes.

* Fri Mar 21 2008 Greg Beeley <Greg.Beeley@LightSys.org> 0.9.0-0
- major development release; see changelog and release notes.

* Fri Apr 07 2006 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.5-0
- major development release; see release notes for details.

* Sat Feb 26 2005 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.4-0
- Initial creation of the RPM.
