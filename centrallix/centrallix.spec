Summary: The Centrallix application platform server.
Name: centrallix
Version: 0.7.4
Release: 0
License: GPL
Group: System Environment/Daemons
Source: centrallix-%{version}.tgz
Buildroot: %{_tmppath}/%{name}-%{version}-root
BuildPrereq: zlib-devel, centrallix-lib-devel = %{version}
Requires: zlib, centrallix-lib = %{version}
URL: http://www.centrallix.net/

%description
The Centrallix package provides the Centrallix application platform
server, an innovative new software development paradigm focused on
object and rule based declarative development, and featuring rich
web-based application deployment with zero client footprint.

%package sybase
Summary: The Sybase ASE ObjectSystem Driver
Group: System Environment/Daemons
Requires: centrallix = %{version}, sybase-ocsd >= 10.0.4-6

%description 
The Sybase ASE ObjectSystem Driver provides Centrallix access to the
Sybase ASE database server via the Open Client / TDS protocol and
ct-library interface.  This is provided as a separate package since
not everyone has sybase-ocsd installed.

%prep
%setup -q

%build
./configure --prefix=$RPM_BUILD_ROOT/usr --enable-optimization --enable-sybase --with-logmethod=syslog
make
make modules
make config

%pre
echo "**** Centrallix Application Platform Installation **** PLEASE READ ****"
echo ""
echo "You are installing the Centrallix application platform server.  This"
echo "system is Free Software, and you are welcome to redistribute it under"
echo "certain conditions; see the file COPYING included with this distribution"
echo "for further details.  Centrallix comes AS IS with ABSOLUTELY NO WARRANTY;"
echo "by installing this software, you assume ALL RISKS of its use.  For"
echo "details, see the file COPYING that accompanies this distribution."
echo ""
echo "This version is UNSTABLE DEVELOPMENT software.  It is incomplete and"
echo "has a lot of bugs, including some with potential security ramifications."
echo "At present, we do not advise the use of this software where untrusted"
echo "parties may have access to its services, or where untrusted parties may"
echo "be able to influence or control data that is processed by this software."
echo ""
echo "We realize that install messages like this can be annoying, but we only"
echo "thought it fair to make sure you were aware of the software's status!"
echo ""
echo "If these terms are not fully acceptable to you, you are welcome to"
echo "terminate the installation by pressing CTRL-C now.  Otherwise, just"
echo "press [ENTER] to get started!!"
echo ""
echo -n "[CTRL-C] or [ENTER]: "
read ANSWER

%install
rm -rf $RPM_BUILD_ROOT
make install
mkdir -p /usr/share/doc/centrallix-%{version}
cp COPYING /usr/share/doc/centrallix-%{version}
mkdir -p /var/centrallix/os
echo "<html><head>Centrallix</head><body>Centrallix Successfully Installed.<br><br>Please install a centrallix-os package for full functionality!</body></html>" > /var/centrallix/os/index.html

%files
%defattr(-,root,root)
/usr/sbin/centrallix
/usr/lib/centrallix/objdrv_http.so
/usr/lib/centrallix/objdrv_xml.so
/usr/lib/centrallix/objdrv_mime.so
/usr/lib/centrallix/objdrv_gzip.so
/usr/lib/centrallix/objdrv_mbox.so
/usr/lib/centrallix/objdrv_pop3_v3.so
/usr/share/doc/centrallix-%{version}/COPYING
%verify(not md5 size mtime) %config(noreplace) /etc/centrallix.conf
%verify(not md5 size mtime) %config(noreplace) /etc/rootnode
%config /etc/useragent.cfg
%config /etc/types.cfg
/etc/rc.d/init.d/centrallix

%files sybase
%defattr(-,root,root)
/usr/lib/centrallix/objdrv_sybase.so

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Sat Feb 26 2005 Greg Beeley <Greg.Beeley@LightSys.org> 0.7.4-0
- Initial creation of the RPM.
