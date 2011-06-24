Name: hangwatch
Summary: Triggers a system action if a user-defined loadavg is exceeded
Group: Applications/System
License: GPLv2

Version: 0.3
Release: 17%{?dist}

#Url: http://people.redhat.com/~csnook/hangwatch/
#url: http://people.redhat.com/astokes/hangwatch/
url: http://github.com/jumanjiman/hangwatch

Source: %{name}-%{version}.tar.gz
BuildRoot:  %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildRequires: gcc

Requires: /usr/bin/logger
Requires: /bin/taskset
Requires: /usr/bin/chrt
Requires: grep
Requires: chkconfig
Requires: initscripts

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
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU General Public License for more details.

%prep
%setup -q

%clean
%{__rm} -fr %{buildroot}
make %{?_smp_mflags} -C src/ clean

%build
make %{?_smp_mflags} -C src/ all


%install
%{__rm} -fr %{buildroot}
%{__mkdir_p} %{buildroot}/%{_sbindir}
%{__mkdir_p} %{buildroot}/%{_sysconfdir}/sysconfig
%{__mkdir_p} %{buildroot}/%{_sysconfdir}/rc.d/init.d
%{__mkdir_p} %{buildroot}/%{_var}/run/hangwatch
%{__install} -p -m755 src/hangwatch %{buildroot}/usr/sbin
pushd %{buildroot}/usr/sbin
%{__ln_s} hangwatch hangwatch1
%{__ln_s} hangwatch hangwatch2
%{__ln_s} hangwatch hangwatch3
popd
%{__install} -p -m755 src/etc/rc.d/init.d/hangwatch %{buildroot}/%{_sysconfdir}/rc.d/init.d
%{__install} -p -m644 src/etc/sysconfig/hangwatch %{buildroot}/%{_sysconfdir}/sysconfig

%files
%defattr(-,root,root,-)

%{_sbindir}/hangwatch
%{_sbindir}/hangwatch1
%{_sbindir}/hangwatch2
%{_sbindir}/hangwatch3
%dir %{_var}/run/hangwatch

%config %{_sysconfdir}/rc.d/init.d/hangwatch
%config %{_sysconfdir}/sysconfig/hangwatch

%doc README.asciidoc
%doc src/LICENSE
%doc src/README.first
%doc src/FAQ.orig
%doc src/FAQ.astokes


# =====================================================
# NOTE ON SCRIPTLETS (pre,post,preun,postun)
# -----------------------------------------------------
# RPM upgrade uses the following sequence.
# The number in parentheses is the value of $1
#   Run %pre of new package (2)
#   Install new files
#   Run %post of new package (2)
#   Run %preun of old package (1)
#   Delete any old files not overwritten by newer ones
#   Run %postun of old package (1)
# =====================================================


%preun
if [ $1 -eq 0 ]; then
  /sbin/service hangwatch stop || :
  /sbin/chkconfig --delete hangwatch || :
fi

%post
if [ $1 -gt 0 ]; then
  /sbin/chkconfig hangwatch on
  /bin/grep -q 'ks=' /proc/cmdline
  if [ $? -ne 0 ]; then
    # start if we're not kickstarting
    /sbin/service hangwatch start
  else
    :
  fi
fi

%postun
if [ $1 -gt 0 ]; then
  /sbin/service hangwatch condrestart || :
fi

%changelog
* Sat Aug 28 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-17
- cleaned up spec file (jumanjiman@gmail.com)

* Sat Jul 24 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-16
- expanded and cleaned up README.asciidoc

* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-15
- now supports up to 3 instances of hangwatch with different parameters
- removed (noreplace) from /etc/sysconfig/hangwatch due to major change in
  config file; look for .rpmsave after upgrade
- preliminary support for cpu affinity and process priority
- added taskset and chrt as dependencies; minor cleanup of spec file
- removed reload from init script
- helpful error is reported if /etc/sysconfig/hangwatch is unreadable

* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-14
- hangwatch starts by default except during kickstart

* Fri Jul 23 2010 Paul Morgan <jumanjiman@gmail.com> 0.3-13
- hangwatch now lives in /usr/sbin/

* Fri Jul 23 2010 Paul Morgan <pmorgan@redhat.com> 0.3-7
- updated source from http://people.redhat.com/astokes/hangwatch/
  which daemonizes hangwatch
- adapted spec for tito builds

* Fri Sep 21 2007 Paul Morgan <pmorgan@redhat.com> 0.03-1
- added init script plus config file

* Thu Sep 20 2007 Paul Morgan <pmorgan@redhat.com>
- initial package for convenience
