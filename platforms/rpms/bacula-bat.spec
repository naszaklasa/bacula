# Bacula RPM spec file
#
# Copyright (C) 2000-2011 Free Software Foundation Europe e.V.

# Platform Build Configuration

# basic defines for every build
%define _release           1
%define _version           5.2.0
%define depkgs_qt_version  16Dec10

# this is the QT version in depkgs_qt
%define qt4ver             4.6.2

%define _packager Kern Sibbald <kern@sibbald.com>

%define manpage_ext gz

%define single_dir 0
%{?single_dir_install:%define single_dir 1}

# Installation Directory locations
%if %{single_dir}
%define _prefix        /opt/bacula
%define _sbindir       /opt/bacula/bin
%define _bindir        /opt/bacula/bin
%define _subsysdir     /opt/bacula/working
%define sqlite_bindir  /opt/bacula/sqlite
%define _mandir        /usr/share/man
%define sysconf_dir    /opt/bacula/etc
%define script_dir     /opt/bacula/scripts
%define working_dir    /opt/bacula/working
%define pid_dir        /opt/bacula/working
%define plugin_dir     /opt/bacula/plugins
%define lib_dir        /opt/bacula/lib
%else
%define _prefix        /usr
%define _sbindir       %_prefix/sbin
%define _bindir        %_prefix/bin
%define _subsysdir     /var/lock/subsys
%define sqlite_bindir  %_libdir/bacula/sqlite
%define _mandir        %_prefix/share/man
%define sysconf_dir    /etc/bacula
%define script_dir     %_libdir/bacula
%define working_dir    /var/lib/bacula
%define pid_dir        /var/run
%define plugin_dir     %_libdir/bacula/plugins
%define lib_dir        %_libdir/bacula/lib
%endif

# Daemon user:group Don't change them unless you know what you are doing
%define director_daemon_user    bacula
%define daemon_group            bacula

#--------------------------------------------------------------------------
# it should not be necessary to change anything below here for a release
# except for patch macros in the setup section
#--------------------------------------------------------------------------

%{?contrib_packager:%define _packager %{contrib_packager}}

%{expand: %%define gccver %(rpm -q --queryformat %%{version} gcc)}
%{expand: %%define gccrel %(rpm -q --queryformat %%{release} gcc)}

%define staticqt 1
%{?nobuild_staticqt:%define staticqt 0}

# determine what platform we are building on
%define fedora 0
%define suse 0
%define mdk 0

%if %{_vendor} == redhat
        %define fedora 1
        %define _dist %(cat /etc/redhat-release)
%endif
%if %{_vendor} == suse
        %define suse 1
        %define _dist %(grep -i SuSE /etc/SuSE-release)
%endif
%if %{_vendor} == Mandriva
        %define mdk 1
        %define _dist %(grep Mand /etc/mandrake-release)
%endif
%if ! %{fedora} && ! %{suse} && ! %{mdk}
%{error: Unknown platform. Please examine the spec file.}
exit 1
%endif

Summary: Bacula - The Network Backup Solution
Name: bacula-bat
Version: %{_version}
Release: %{_release}
Group: System Environment/Daemons
License: AGPLv3
BuildRoot: %{_tmppath}/%{name}-root
URL: http://www.bacula.org/
Vendor: The Bacula Team
Packager: %{_packager}
Prefix: %{_prefix}
Distribution: %{_dist}

Source0: http://www.prdownloads.sourceforge.net/bacula/bacula-%{version}.tar.gz
Source1: http://www.prdownloads.sourceforge.net/bacula/depkgs-qt-%{depkgs_qt_version}.tar.gz

BuildRequires: gcc, gcc-c++, make, autoconf
BuildRequires: libstdc++-devel = %{gccver}-%{gccrel}, zlib-devel
BuildRequires: openssl-devel, fontconfig-devel, libpng-devel, libstdc++-devel, zlib-devel

Requires: openssl
Requires: fontconfig
Requires: libgcc
Requires: libpng
Requires: libstdc++
Requires: zlib
Requires: bacula-libs

%if %{suse}
Requires: /usr/bin/kdesu
Requires: freetype2
BuildRequires: freetype2-devel
%else
Requires: usermode
Requires: freetype
BuildRequires: freetype-devel
%endif

# Source directory locations
%define depkgs_qt ../depkgs-qt

# define the basic package description
%define blurb Bacula - The Leading Open Source Backup Solution.
%define blurb2 Bacula is a set of computer programs that permit you (or the system
%define blurb3 administrator) to manage backup, recovery, and verification of computer
%define blurb4 data across a network of computers of different kinds. In technical terms,
%define blurb5 it is a network client/server based backup program. Bacula is relatively
%define blurb6 easy to use and efficient, while offering many advanced storage management
%define blurb7 features that make it easy to find and recover lost or damaged files.
%define blurb8 Bacula source code has been released under the AGPL version 3 license.

Summary: Bacula - The Network Backup Solution
Group: System Environment/Daemons

%description
%{blurb}

%{blurb2}
%{blurb3}
%{blurb4}
%{blurb5}
%{blurb6}
%{blurb7}
%{blurb8}

This is the Bacula Administration Tool (bat) graphical user interface package.
It is an add-on to the client or server packages.

%prep
%setup -T -n bacula-%{_version} -b 0
%setup -T -D -n bacula-%{_version} -b 1

%build


cwd=${PWD}
%if ! %{staticqt}
export QTDIR=$(pkg-config --variable=prefix QtCore)
export QTINC=$(pkg-config --variable=includedir QtCore)
export QTLIB=$(pkg-config --variable=libdir QtCore)
export PATH=${QTDIR}/bin/:${PATH}
%else
cd %{depkgs_qt}
make qt4 <<EOF
yes
EOF
qtdir=${PWD}
export PATH=${qtdir}/qt4/bin:$PATH
export QTDIR=${qtdir}/qt4/
export QTINC=${qtdir}/qt4/include/
export QTLIB=${qtdir}/qt4/lib/
export QMAKESPEC=${qtdir}/qt-x11-opensource-src-%{qt4ver}/mkspecs/linux-g++/
cd ${cwd}
%endif

# Main Bacula configuration with bat
%configure \
        --prefix=%{_prefix} \
        --sbindir=%{_sbindir} \
        --sysconfdir=%{sysconf_dir} \
        --mandir=%{_mandir} \
        --with-scriptdir=%{script_dir} \
        --with-working-dir=%{working_dir} \
        --with-plugindir=%{script_dir} \
        --with-pid-dir=%{pid_dir} \
        --with-subsys-dir=%{_subsysdir} \
        --enable-smartalloc \
        --enable-bat \
        --without-qwt \
        --enable-client-only \
        --with-dir-user=%{director_daemon_user} \
        --with-dir-group=%{daemon_group} \
        --with-sd-user=%{storage_daemon_user} \
        --with-sd-group=%{storage_daemon_group} \
        --with-fd-user=%{file_daemon_user} \
        --with-fd-group=%{daemon_group} \
        --with-dir-password="XXX_REPLACE_WITH_DIRECTOR_PASSWORD_XXX" \
        --with-fd-password="XXX_REPLACE_WITH_CLIENT_PASSWORD_XXX" \
        --with-sd-password="XXX_REPLACE_WITH_STORAGE_PASSWORD_XXX" \
        --with-mon-dir-password="XXX_REPLACE_WITH_DIRECTOR_MONITOR_PASSWORD_XXX" \
        --with-mon-fd-password="XXX_REPLACE_WITH_CLIENT_MONITOR_PASSWORD_XXX" \
        --with-mon-sd-password="XXX_REPLACE_WITH_STORAGE_MONITOR_PASSWORD_XXX" \
        --with-openssl

make

%install
mkdir -p $RPM_BUILD_ROOT/usr/share/applications
mkdir -p $RPM_BUILD_ROOT/usr/share/pixmaps
%if ! %{suse}
mkdir -p $RPM_BUILD_ROOT/etc/pam.d
mkdir -p $RPM_BUILD_ROOT/etc/security/console.apps
mkdir -p $RPM_BUILD_ROOT%{_sbindir}
mkdir -p $RPM_BUILD_ROOT/usr/bin
%endif

cd src/qt-console
make DESTDIR=$RPM_BUILD_ROOT install
cd ../..

rm -rf $RPM_BUILD_ROOT%{_prefix}/share/doc/bacula

%if %{suse}
cp -p src/qt-console/images/bat_icon.png $RPM_BUILD_ROOT/usr/share/pixmaps/bat_icon.png
cp -p scripts/bat.desktop.xsu $RPM_BUILD_ROOT/usr/share/applications/bat.desktop
touch $RPM_BUILD_ROOT%{sysconf_dir}/bat.kdesu
%else
cp -p src/qt-console/images/bat_icon.png $RPM_BUILD_ROOT/usr/share/pixmaps/bat_icon.png
cp -p scripts/bat.desktop.consolehelper $RPM_BUILD_ROOT/usr/share/applications/bat.desktop
cp -p scripts/bat.console_apps $RPM_BUILD_ROOT/etc/security/console.apps/bat
cp -p scripts/bat.pamd $RPM_BUILD_ROOT/etc/pam.d/bat
ln -sf consolehelper $RPM_BUILD_ROOT/usr/bin/bat
%endif

%files
%defattr(-,root,root)
%attr(-, root, %{daemon_group}) %{_sbindir}/bat
%attr(-, root, %{daemon_group}) %dir %{sysconf_dir}
%attr(-, root, %{daemon_group}) %config(noreplace) %{sysconf_dir}/bat.conf
/usr/share/pixmaps/bat_icon.png
/usr/share/applications/bat.desktop

# if user is a member of daemon_group then kdesu will run bat as user
%if %{suse}
%attr(0660, root, %{daemon_group}) %{sysconf_dir}/bat.kdesu
%endif

%if ! %{suse}
# add the console helper files
%config(noreplace,missingok) /etc/pam.d/bat
%config(noreplace,missingok) /etc/security/console.apps/bat
/usr/bin/bat
%endif

%pre
# create the daemon group
HAVE_BACULA=`grep %{daemon_group} %{group_file} 2>/dev/null`
if [ -z "$HAVE_BACULA" ]; then
    %{groupadd} -r %{daemon_group} > /dev/null 2>&1
    echo "The group %{daemon_group} has been added to %{group_file}."
    echo "See the manual chapter \"Running Bacula\" for details."
fi


%post
if [ -d %{sysconf_dir} ]; then
   cd %{sysconf_dir}
   for string in XXX_REPLACE_WITH_DIRECTOR_PASSWORD_XXX XXX_REPLACE_WITH_CLIENT_PASSWORD_XXX XXX_REPLACE_WITH_STORAGE_PASSWORD_XXX XXX_REPLACE_WITH_DIRECTOR_MONITOR_PASSWORD_XXX XXX_REPLACE_WITH_CLIENT_MONITOR_PASSWORD_XXX XXX_REPLACE_WITH_STORAGE_MONITOR_PASSWORD_XXX; do
      pass=`openssl rand -base64 33`
      for file in *.conf; do
         need_password=`grep ${string} $file 2>/dev/null`
         if [ -n "$need_password" ]; then
            sed "s@${string}@${pass}@g" $file > $file.new
            cp -f $file.new $file; rm -f $file.new
         fi
      done
   done
# put actual hostname in conf file
   host=`hostname`
   string="XXX_HOSTNAME_XXX"
   for file in *.conf; do
      need_host=`grep ${string} $file 2>/dev/null`
      if [ -n "$need_host" ]; then
         sed "s@${string}@${host}@g" $file >$file.new
         cp -f $file.new $file; rm -f $file.new
      fi
   done
fi
/sbin/ldconfig

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"
rm -rf $RPM_BUILD_DIR/depkgs-qt

%changelog
* Sun Mar 14 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- Fix for QT mkspecs location on FC12
- allow user to build without embedded static QT
* Sat Feb 27 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- add dependency on bacula-libs
* Sat Feb 13 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- create file to allow bat to run nonroot with kdesu
- add dependency information
* Sat Jan 30 2010 D. Scott Barninger <barninger@fairfieldcomputers.com>
- fix consolehelper/xsu for suse packages
* Sat Aug 1 2009 Kern Sibbald <kern@sibbald.com>
- Split bat into separate bacula-bat.spec
