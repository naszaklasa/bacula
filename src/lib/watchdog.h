/*
 * Watchdog timer routines
 *
 *    Kern Sibbald, December MMII
 *
*/
/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

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

enum {
   TYPE_CHILD = 1,
   TYPE_PTHREAD,
   TYPE_BSOCK
};

#define TIMEOUT_SIGNAL SIGUSR2

struct s_watchdog_t {
	bool one_shot;
	time_t interval;
	void (*callback)(struct s_watchdog_t *wd);
	void (*destructor)(struct s_watchdog_t *wd);
	void *data;
	/* Private data below - don't touch outside of watchdog.c */
	dlink link;
	time_t next_fire;
};
typedef struct s_watchdog_t watchdog_t;
