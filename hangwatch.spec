Name: hangwatch
Version: 0.3
Release: 12%{?dist}
#Url: http://people.redhat.com/~csnook/hangwatch/
#url: http://people.redhat.com/astokes/hangwatch/
url: http://github.com/jumanjiman/hangwatch
Summary: Triggers a system action if a user-defined loadavg is exceeded
Group: Performance Tools
License: GPL v2
Source: %{name}-%{version}.tar.gz
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

Many hangs are actually the result of very rapid load spikes
which never show up in logs, because the applications that would
be logging the load spike cannot run. Calling it "hangwatch"
convinces people to run it even if they haven't seen load spikes
with their hangs.

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
make -C src/ clean

%build
[ "%{buildroot}" = "/" ] && exit 1
rm -fr %{buildroot}
make -C src/ all


%install
mkdir -p %{buildroot}/sbin
mkdir -p %{buildroot}/etc/sysconfig
mkdir -p %{buildroot}/etc/rc.d/init.d
install -m755 src/hangwatch %{buildroot}/sbin
install -m755 src/etc/rc.d/init.d/hangwatch %{buildroot}/etc/rc.d/init.d
install -m644 src/etc/sysconfig/hangwatch %{buildroot}/etc/sysconfig
mkdir -p %{buildroot}/var/run/hangwatch

%files
%defattr(-,root,root,-)
/sbin/hangwatch
/var/run/hangwatch
%config /etc/rc.d/init.d/hangwatch
%config(noreplace) /etc/sysconfig/hangwatch
%doc README.asciidoc
%doc src/LICENSE
%doc src/README.first
%doc src/FAQ.orig
%doc src/FAQ.astokes

%preun
if [ $1 -eq 0 ]; then
  /sbin/service hangwatch stop
  /sbin/chkconfig --delete hangwatch
fi

%changelog
* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-12
- fixed files section of spec due to name change (jumanjiman@gmail.com)

* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-11
- fixed install section to account for tio src tree (jumanjiman@gmail.com)

* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-10
- adapted Makefile for tito src tree (jumanjiman@gmail.com)

* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-9
- really changed source spec for tito (jumanjiman@gmail.com)

* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-8
- changed source spec for tito (jumanjiman@gmail.com)

* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-7
- new package built with tito

* Fri Jul 23 2010 Paul Morgan <pmorgan@redhat.com> 0.3-7
- updated source from http://people.redhat.com/astokes/hangwatch/
  which daemonizes hangwatch
- adapted spec for tito builds

* Fri Sep 21 2007 Paul Morgan <pmorgan@redhat.com> 0.03-1
- added init script plus config file

* Thu Sep 20 2007 Paul Morgan <pmorgan@redhat.com>
- initial package for convenience
