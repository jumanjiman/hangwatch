Name: hangwatch
Version: 0.3
Release: 7%{?dist}
#Url: http://people.redhat.com/~csnook/hangwatch/
#url: http://people.redhat.com/astokes/hangwatch/
url: http://github.com/jumanjiman/hangwatch
Summary: Triggers a system action if a user-defined loadavg is exceeded
Group: Performance Tools
License: GPL v2
Source: hangwatch-%{version}-%{release}.tgz
Packager: Paul Morgan <jumanjiman@gmail.com>
BuildRoot: /tmp/%{name}-%{version}-%{release}
BuildRequires: gcc

Requires: /usr/bin/logger

%description
Hangwatch periodically polls /proc/loadavg, and echos a user-defined
set of characters into /proc/sysrq-trigger if a user-defined load
threshold is exceeded. Hangwatch will fire when the system is too
bogged down to get work done, but probably won't help for a hard
lockup.

Thanks to Chris Snook for making this tool available.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

%prep
%setup -q

%clean
[ "%{buildroot}" = "/" ] && exit 1
rm -fr %{buildroot}

%build
[ "%{buildroot}" = "/" ] && exit 1
rm -fr %{buildroot}
make all

%install
mkdir -p %{buildroot}/sbin
mkdir -p %{buildroot}/etc/sysconfig
mkdir -p %{buildroot}/etc/rc.d/init.d
install -m755 hangwatch %{buildroot}/sbin
install -m755 etc/rc.d/init.d/hangwatch %{buildroot}/etc/rc.d/init.d
install -m644 etc/sysconfig/hangwatch %{buildroot}/etc/sysconfig
mkdir -p %{buildroot}/var/run/hangwatch

%files
%defattr(-,root,root,-)
/sbin/hangwatch
/var/run/hangwatch
%config /etc/rc.d/init.d/hangwatch
%config(noreplace) /etc/sysconfig/hangwatch
%doc README.asciidoc
%doc src/LICENSE

%preun
if [ $1 -eq 0 ]; then
  /sbin/service hangwatch stop
  /sbin/chkconfig --delete hangwatch
fi

%changelog
* Fri Sep 21 2007 Paul Morgan <pmorgan@redhat.com>
- added init script plus config file

* Thu Sep 20 2007 Paul Morgan <pmorgan@redhat.com>
- initial package for convenience
