#!/bin/bash

# shell script to build bacula rpm release
# copy this script into a working directory with the src rpm to build and execute
# 19 Aug 2006 D. Scott Barninger

# Copyright (C) 2006 Free Software Foundation Europe e.V.
# licensed under GPL-v2

# signing rpms
# Make sure you have a .rpmmacros file in your home directory containing the following:
#
# %_signature gpg
# %_gpg_name Your Name <your-email@site.org>
#
# the %_gpg_name information must match your key


# usage: ./build_rpm.sh

###########################################################################################
# script configuration section

VERSION=2.2.5
RELEASE=1

# build platform for spec
# set to one of rh7,rh8,rh9,fc1,fc3,fc4,fc5,fc6,fc7,wb3,rhel3,rhel4,rhel5,centos3,centos4,centos5,sl3, sl4,sl5,su9,su10,su102,su103,mdk,mdv
PLATFORM=su102

# platform designator for file names
# for RedHat/Fedora set to one of rh7,rh8,rh9,fc1,fc3,fc4,fc5,fc6,fc7 OR
# for RHEL3/clones wb3, rhel3, sl3 & centos3 set to el3 OR
# for RHEL4/clones rhel4, sl4 & centos4 set to el4 OR
# for RHEL5/clones rhel5, sl5 & centos5 set to el5 OR
# for SuSE set to su90, su91, su92, su100 or su101 or su102 or su103 OR
# for Mandrake set to 101mdk or 20060mdk
FILENAME=su102

# MySQL version
# set to empty (for MySQL 3), 4 or 5
MYSQL=5

# building wxconsole
# set to 1 to build wxconsole package else set 0
WXCONSOLE=0

# building bat
# set to 1 to build bat package else set 0
BAT=0

# enter your name and email address here
PACKAGER="Your Name <your-email@site.org>"

# enter the full path to your RPMS output directory
RPMDIR=/usr/src/packages/RPMS/i586

# enter the full path to your rpm BUILD directory
RPMBUILD=/usr/src/packages/BUILD

# enter your arch string here (i386, i586, i686, x86_64)
ARCH=i586

# if the src rpm is not in the current working directory enter the directory location
# with trailing slash where it is found.
SRPMDIR=

# set to 1 to sign packages, 0 not to sign if you want to sign on another machine.
SIGN=1

# to save the bacula-updatedb package set to 1, else 0
# only one updatedb package is required per release so normally this should be 0
# for all contrib packagers
SAVEUPDATEDB=0

# to override your language shell variable uncomment and edit this
# export LANG=en_US.UTF-8

# Make no changes below this point without consensus

############################################################################################

SRPM=${SRPMDIR}bacula-$VERSION-$RELEASE.src.rpm

echo Building MySQL packages for "$PLATFORM"...
sleep 2
if [ "$WXCONSOLE" = "1" ]; then
rpmbuild --rebuild --define "build_${PLATFORM} 1" \
--define "build_mysql${MYSQL} 1" \
--define "contrib_packager ${PACKAGER}" \
--define "build_python 1" \
--define "build_wxconsole 1" \
${SRPM}
else
rpmbuild --rebuild --define "build_${PLATFORM} 1" \
--define "build_mysql${MYSQL} 1" \
--define "build_python 1" \
--define "contrib_packager ${PACKAGER}" ${SRPM}
fi
rm -rf ${RPMBUILD}/*

echo Building PostgreSQL packages for "$PLATFORM"...
sleep 2
if [ "$BAT" = "1" ]; then
rpmbuild --rebuild --define "build_${PLATFORM} 1" \
--define "build_postgresql 1" \
--define "contrib_packager ${PACKAGER}" \
--define "build_python 1" \
--define "build_bat 1" \
--define "nobuild_gconsole 1" ${SRPM}
else
rpmbuild --rebuild --define "build_${PLATFORM} 1" \
--define "build_postgresql 1" \
--define "contrib_packager ${PACKAGER}" \
--define "build_python 1" \
--define "nobuild_gconsole 1" ${SRPM}
fi
rm -rf ${RPMBUILD}/*

echo Building SQLite packages for "$PLATFORM"...
sleep 2
rpmbuild --rebuild --define "build_${PLATFORM} 1" \
--define "build_sqlite 1" \
--define "contrib_packager ${PACKAGER}" \
--define "build_python 1" \
--define "nobuild_gconsole 1" ${SRPM}
rm -rf ${RPMBUILD}/*

# delete the updatedb package and any debuginfo packages built
rm -f ${RPMDIR}/bacula*debug*
if [ "$SAVEUPDATEDB" = "1" ]; then
        mv -f ${RPMDIR}/bacula-updatedb* ./;
else
        rm -f ${RPMDIR}/bacula-updatedb*;
fi

# copy files to cwd and rename files to final upload names

mv -f ${RPMDIR}/bacula-mysql-${VERSION}-${RELEASE}.${ARCH}.rpm \
./bacula-mysql-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/bacula-postgresql-${VERSION}-${RELEASE}.${ARCH}.rpm \
./bacula-postgresql-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/bacula-sqlite-${VERSION}-${RELEASE}.${ARCH}.rpm \
./bacula-sqlite-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/bacula-mtx-${VERSION}-${RELEASE}.${ARCH}.rpm \
./bacula-mtx-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/bacula-client-${VERSION}-${RELEASE}.${ARCH}.rpm \
./bacula-client-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/bacula-gconsole-${VERSION}-${RELEASE}.${ARCH}.rpm \
./bacula-gconsole-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/bacula-wxconsole-${VERSION}-${RELEASE}.${ARCH}.rpm \
./bacula-wxconsole-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/bacula-bat-${VERSION}-${RELEASE}.${ARCH}.rpm \
./bacula-bat-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

# now sign the packages
if [ "$SIGN" = "1" ]; then
        echo Ready to sign packages...;
        sleep 2;
        rpm --addsign ./*.rpm;
fi

echo
echo Finished.
echo
ls

# changelog
# 16 Jul 2006 initial release
# 05 Aug 2006 add python support
# 06 Aug 2006 add remote source directory, add switch for signing, refine file names
# 19 Aug 2006 add $LANG override to config section per request Felix Schwartz
# 27 Jan 2007 add fc6 target
# 29 Apr 2007 add sl3 & sl4 target and bat package
# 06 May 2007 add fc7 target
# 15 Sep 2007 add rhel5 and clones
# 10 Nov 2007 add su103

