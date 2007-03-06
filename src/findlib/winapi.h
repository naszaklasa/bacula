/*
 * Windows APIs that are different for each system.
 *   We use pointers to the entry points so that a
 *   single binary will run on all Windows systems.
 *
 *     Kern Sibbald MMIII
 */
/*
   Copyright (C) 2000-2003 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#ifndef __WINAPI_H
#define __WINAPI_H
#if defined(HAVE_WIN32)
/* Commented out native.h include statement, which is not distributed with the 
 * free version of VC++, and which is not used in bacula. */
/*#if !defined(HAVE_MINGW) // native.h not present on mingw
#include <native.h>
#endif*/
#include <windef.h>
#endif
#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)
/* In ADVAPI32.DLL */

typedef BOOL (WINAPI * t_OpenProcessToken)(HANDLE, DWORD, PHANDLE);
typedef BOOL (WINAPI * t_AdjustTokenPrivileges)(HANDLE, BOOL,
          PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);
typedef BOOL (WINAPI * t_LookupPrivilegeValue)(LPCTSTR, LPCTSTR, PLUID);

extern t_OpenProcessToken      p_OpenProcessToken;
extern t_AdjustTokenPrivileges p_AdjustTokenPrivileges;
extern t_LookupPrivilegeValue  p_LookupPrivilegeValue;

/* In KERNEL32.DLL */
typedef BOOL (WINAPI * t_GetFileAttributesEx)(LPCTSTR, GET_FILEEX_INFO_LEVELS,
       LPVOID);
typedef BOOL (WINAPI * t_SetProcessShutdownParameters)(DWORD, DWORD);
typedef BOOL (WINAPI * t_BackupRead)(HANDLE,LPBYTE,DWORD,LPDWORD,BOOL,BOOL,LPVOID*);
typedef BOOL (WINAPI * t_BackupWrite)(HANDLE,LPBYTE,DWORD,LPDWORD,BOOL,BOOL,LPVOID*);

extern t_GetFileAttributesEx   p_GetFileAttributesEx;
extern t_SetProcessShutdownParameters p_SetProcessShutdownParameters;
extern t_BackupRead         p_BackupRead;
extern t_BackupWrite        p_BackupWrite;


#endif

#endif /* __WINAPI_H */
