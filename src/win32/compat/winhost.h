/*
 * Define Host machine
 *
 *  Version $Id$
 *
 */
/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/

#include "host.h"
#undef HOST_OS
#undef DISTNAME
#undef DISTVER

#ifdef HAVE_MINGW

#define HOST_OS  "Linux"
#define DISTNAME "Cross-compile"
#define BACULA "Bacula"
#ifdef _WIN64
# define DISTVER "Win64"
#else
# define DISTVER "Win32"
#endif

#else

extern DLL_IMP_EXP char WIN_VERSION_LONG[];
extern DLL_IMP_EXP char WIN_VERSION[];

#define HOST_OS  WIN_VERSION_LONG
#define DISTNAME "MVS"
#define DISTVER  WIN_VERSION

#endif
