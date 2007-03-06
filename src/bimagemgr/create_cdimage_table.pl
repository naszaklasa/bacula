#!/usr/bin/perl
##
# create_cdimage_table.pl
# create the bacula table for CD image management
#
# Copyright (C) 2004 D. Scott Barninger
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this program; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307, USA.
##

$VERSION = "0.2.1";

require 5.000; use strict 'vars', 'refs', 'subs';
use DBI;

#------------------------------------------------------------------------------------
# configuration section
my($dbh,$sql,$database,$host,$user,$password,$db_driver,$db_name_param);
$database = "bacula";
$host = "localhost";
$user = "bacula";
$password = "";
## database driver selection - uncomment one set
# MySQL
$db_driver = "mysql";
$db_name_param = "database";
# Postgresql
# $db_driver = "Pg";
# $db_name_param = "dbname";
##
# end configuration section
#------------------------------------------------------------------------------------

# connect to the database
$dbh = DBI->connect("DBI:$db_driver:$db_name_param=$database;host=$host","$user","$password",{'RaiseError' => 1}) || die("Unable to connect to database.");

if ($db_driver eq "mysql") {
$sql = "CREATE TABLE CDImages (
				MediaId INTEGER UNSIGNED NOT NULL,
				LastBurn DATETIME NOT NULL,
				PRIMARY KEY (MediaId))";
}

if ($db_driver eq "Pg") {
$sql = "CREATE TABLE CDImages (
				MediaId integer not null,
   				LastBurn timestamp without time zone not null,
   				primary key (MediaId))";
}

$dbh->do($sql);

$dbh->disconnect();
print "\nFinished creating the CDImages table.\n";
exit;

#-------------------------------------------------------------------#
# Changelog
#
# 0.2 14 Aug 2004
# first functional version
#
# 0.2.1 15 Aug 2004
# add configuration option for Postgresql driver

