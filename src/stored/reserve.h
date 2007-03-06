/*
 * Definitions for reservation system.
 *
 *  Kern Sibbald, February MMVI
 *
 *  Version $Id: reserve.h,v 1.2.2.2 2006/03/14 21:41:45 kerns Exp $
 */ 

/*
   Copyright (C) 2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

/*
 *   Use Device command from Director
 *   The DIR tells us what Device Name to use, the Media Type,
 *      the Pool Name, and the Pool Type.
 *
 *    Ensure that the device exists and is opened, then store
 *      the media and pool info in the JCR.  This class is used
 *      only temporarily in this file.
 */
class DIRSTORE {
public:
   alist *device;
   bool append;
   char name[MAX_NAME_LENGTH];
   char media_type[MAX_NAME_LENGTH];
   char pool_name[MAX_NAME_LENGTH];
   char pool_type[MAX_NAME_LENGTH];
};

/* Reserve context */
class RCTX {
public:
   JCR *jcr;
   char *device_name;
   DIRSTORE *store;
   DEVRES   *device;
   DEVICE *low_use_drive;             /* Low use drive candidate */
   int num_writers;                   /* for selecting low use drive */
   bool try_low_use_drive;            /* see if low use drive available */
   bool any_drive;                    /* Accept any drive if set */
   bool PreferMountedVols;            /* Prefer volumes already mounted */
   bool exact_match;                  /* Want exact volume */
   bool have_volume;                  /* Have DIR suggested vol name */
   bool suitable_device;              /* at least one device is suitable */
   bool autochanger_only;             /* look at autochangers only */
   bool notify_dir;                   /* Notify DIR about device */
   char VolumeName[MAX_NAME_LENGTH];  /* Vol name suggested by DIR */
};
