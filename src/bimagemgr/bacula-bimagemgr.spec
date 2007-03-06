# Platform defines

%define rhel 0
%{?build_rhel:%define rhel 1}

%define suse 0
%{?build_suse:%define suse 1}

%define mdk 0
%{?build_mdk:%define mdk 1}

# test for a platform definition
%if ! %{rhel} && ! %{suse} && ! %{mdk}
%{error: You must specify a platform. Please examine the spec file.}
exit 1
%endif

# set destination directories
%define cgidir /var/www/cgi-bin
%define docdir /var/www/html
%define sysconfdir /etc/bacula
%if %{suse}
define cgidir /srv/www/cgi-bin
define docdir /srv/www/htdocs
%endif

# set ownership of files
%define binowner root
%define bingroup root
%define dataowner apache
%define datagroup apache
%if %{suse}
define dataowner wwwrun
define datagroup www
%endif

Summary: Bacula - The Network Backup Solution
Name: bacula
Version: 1.36.3
Release: 1
Group: System Environment/Daemons
Copyright: GPL v2
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
URL: http://www.bacula.org/
BuildArchitectures: noarch
Vendor: The Bacula Team
Distribution: The Bacula Team 
Packager: D. Scott Barninger <barninger at fairfieldcomputers dot com>

%description

%package bimagemgr
Summary: Bacula - The Network Backup Solution
Group: System Environment/Daemons

Requires: perl, perl-DBI, bacula-server, cdrecord, mkisofs
%if %{mdk}
Requires: apache
%else
Requires: httpd
%endif

%description bimagemgr
Bacula is a set of computer programs that permit you (or the system 
administrator) to manage backup, recovery, and verification of computer 
data across a network of computers of different kinds. bimagemgr is a 
utility to manage backups made to files intended for burning to CDR 
disk. bimagemgr allows you to easily see which Volumes have been written 
to more recently than they have been recorded to CDR disk and record those 
which have.


%prep

%setup 

%build

%install

[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

mkdir -p $RPM_BUILD_ROOT/%{cgidir}
mkdir -p $RPM_BUILD_ROOT/%{docdir}
mkdir -p $RPM_BUILD_ROOT/%{sysconfdir}

cp -p src/bimagemgr/bimagemgr.pl $RPM_BUILD_ROOT/%{cgidir}/
cp -p src/bimagemgr/create_cdimage_table.pl $RPM_BUILD_ROOT/%{sysconfdir}/
cp -p src/bimagemgr/README $RPM_BUILD_ROOT/%{sysconfdir}/README.bimagemgr
cp -p src/bimagemgr/bimagemgr.gif $RPM_BUILD_ROOT/%{docdir}/
cp -p src/bimagemgr/cdrom_spins.gif $RPM_BUILD_ROOT/%{docdir}/
cp -p src/bimagemgr/clearpixel.gif $RPM_BUILD_ROOT/%{docdir}/
cp -p src/bimagemgr/temp.html $RPM_BUILD_ROOT/%{docdir}/

chmod 755 $RPM_BUILD_ROOT/%{cgidir}/bimagemgr.pl
chmod 750 $RPM_BUILD_ROOT/%{sysconfdir}/create_cdimage_table.pl
chmod 644 $RPM_BUILD_ROOT/%{sysconfdir}/README.bimagemgr
chmod 644 $RPM_BUILD_ROOT/%{docdir}/*
chmod 664 $RPM_BUILD_ROOT/%{docdir}/temp.html

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"

%files bimagemgr
%defattr(-,%{binowner},%{bingroup})
%{cgidir}/bimagemgr.pl
%{sysconfdir}/create_cdimage_table.pl
%{sysconfdir}/README.bimagemgr
%{docdir}/bimagemgr.gif
%{docdir}/cdrom_spins.gif
%{docdir}/clearpixel.gif

%defattr(-,%{dataowner},%{datagroup})
%{docdir}/temp.html

%post bimagemgr

%preun bimagemgr

%changelog
* Sun Nov 14 2004 D. Scott Barninger <barninger at fairfieldcomputers.com>
- initial spec file
