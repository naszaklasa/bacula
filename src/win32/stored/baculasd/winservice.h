//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file was part of the ups system.
//
//  The ups system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the ups system is not available from the place
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on ups@uk.research.att.com for information on obtaining it.
//
// This file has been adapted to the Win32 version of Bacula
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Copyright (C) 2000-2006 Free Software Foundation Europe e.V.
//


// winservice.cpp

// SERVICE-MODE CODE

// This class provides access to service-oriented routines, under both
// Windows NT and Windows 95.  Some routines only operate under one
// OS, others operate under any OS.

#if (!defined(_win_bacService))
#define _win_bacService

// The NT-specific code wrapper class
class bacService
{
public:
        bacService();

   // SERVICE INSTALL & START FUNCTIONS

   // Routine called by WinMain to cause Bacula to be installed
   // as a service.
   static int BaculaServiceMain();

   // Routine to install the Apcupsd service on the local machine
   static int InstallService(const char *pszCmdLine);

   // Routine to remove the Apcupsd service from the local machine
   static int RemoveService();

   // SERVICE SUPPORT FUNCTIONS

   // Routine to establish whether the current instance is running
   // as a service or not
   static BOOL RunningAsService();

   // Routine to kill any other running copy of Apcupsd
   static BOOL KillRunningCopy();

   // Routine to make the an already running copy of Apcupsd bring up its
   // About box so you can check the version!
   static BOOL ShowAboutBox();

   // Routine to make the an already running copy of Apcupsd bring up its
   // Status dialog
   static BOOL ShowStatus();
};

#endif
