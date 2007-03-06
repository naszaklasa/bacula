/*
 *   edit.c  edit string to ascii, and ascii to internal
 *
 *    Kern Sibbald, December MMII
 *
 *   Version $Id: edit.c,v 1.20.4.1 2005/02/14 10:02:25 kerns Exp $
 */

/*
   Copyright (C) 2000-2005 Kern Sibbald

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

#include "bacula.h"
#include <math.h>

/* We assume ASCII input and don't worry about overflow */
uint64_t str_to_uint64(char *str)
{
   register char *p = str;
   register uint64_t value = 0;

   if (!p) {
      return 0;
   }
   while (B_ISSPACE(*p)) {
      p++;
   }
   if (*p == '+') {
      p++;
   }
   while (B_ISDIGIT(*p)) {
      value = value * 10 + *p - '0';
      p++;
   }
   return value;
}

int64_t str_to_int64(char *str)
{
   register char *p = str;
   register int64_t value;
   bool negative = false;

   if (!p) {
      return 0;
   }
   while (B_ISSPACE(*p)) {
      p++;
   }
   if (*p == '+') {
      p++;
   } else if (*p == '-') {
      negative = true;
      p++;
   }
   value = str_to_uint64(p);
   if (negative) {
      value = -value;
   }
   return value;
}


/*
 * Edit an integer number with commas, the supplied buffer
 * must be at least 27 bytes long.  The incoming number
 * is always widened to 64 bits.
 */
char *edit_uint64_with_commas(uint64_t val, char *buf)
{
   /*
    * Replacement for sprintf(buf, "%" llu, val)
    */
   char mbuf[50];
   mbuf[sizeof(mbuf)-1] = 0;
   int i = sizeof(mbuf)-2;		   /* edit backward */
   if (val == 0) {
      mbuf[i--] = '0';
   } else {
      while (val != 0) {
         mbuf[i--] = "0123456789"[val%10];
	 val /= 10;
      }
   }
   strcpy(buf, &mbuf[i+1]);
   return add_commas(buf, buf);
}

/*
 * Edit an integer number, the supplied buffer
 * must be at least 27 bytes long.  The incoming number
 * is always widened to 64 bits.
 */
char *edit_uint64(uint64_t val, char *buf)
{
   /*
    * Replacement for sprintf(buf, "%" llu, val)
    */
   char mbuf[50];
   mbuf[sizeof(mbuf)-1] = 0;
   int i = sizeof(mbuf)-2;		   /* edit backward */
   if (val == 0) {
      mbuf[i--] = '0';
   } else {
      while (val != 0) {
         mbuf[i--] = "0123456789"[val%10];
	 val /= 10;
      }
   }
   strcpy(buf, &mbuf[i+1]);
   return buf;
}

char *edit_int64(int64_t val, char *buf)
{
   /*
    * Replacement for sprintf(buf, "%" llu, val)
    */
   char mbuf[50];
   bool negative = false;
   mbuf[sizeof(mbuf)-1] = 0;
   int i = sizeof(mbuf)-2;		   /* edit backward */
   if (val == 0) {
      mbuf[i--] = '0';
   } else {
      if (val < 0) {
	 negative = true;
	 val = -val;
      }
      while (val != 0) {
         mbuf[i--] = "0123456789"[val%10];
	 val /= 10;
      }
   }
   if (negative) {
      mbuf[i--] = '-';
   }
   strcpy(buf, &mbuf[i+1]);
   return buf;
}


/*
 * Given a string "str", separate the integer part into
 *   str, and the modifier into mod.
 */
static bool get_modifier(char *str, char *num, int num_len, char *mod, int mod_len)
{
   int i, len, num_begin, num_end, mod_begin, mod_end;

   /*
    * Look for modifier by walking back looking for the first
    *	space or digit.
    */
   strip_trailing_junk(str);
   len = strlen(str);

   for (i=0; i<len; i++) {
      if (!B_ISSPACE(str[i])) {
	 break;
      }
   }
   num_begin = i;

   /* Walk through integer part */
   for ( ; i<len; i++) {
      if (!B_ISDIGIT(str[i])) {
	 break;
      }
   }
   num_end = i;
   if (num_len > (num_end - num_begin + 1)) {
      num_len = num_end - num_begin + 1;
   }
   if (num_len == 0) {
      return false;
   }
   for ( ; i<len; i++) {
      if (!B_ISSPACE(str[i])) {
	 break;
      }
   }
   mod_begin = i;
   for ( ; i<len; i++) {
      if (!B_ISALPHA(str[i])) {
	 break;
      }
   }
   mod_end = i;
   if (mod_len > (mod_end - mod_begin + 1)) {
      mod_len = mod_end - mod_begin + 1;
   }
   Dmsg5(900, "str=%s: num_beg=%d num_end=%d mod_beg=%d mod_end=%d\n",
      str, num_begin, num_end, mod_begin, mod_end);
   bstrncpy(num, &str[num_begin], num_len);
   bstrncpy(mod, &str[mod_begin], mod_len);
   if (!is_a_number(num)) {
      return false;
   }
   bstrncpy(str, &str[mod_end], len);

   return true;
}

/*
 * Convert a string duration to utime_t (64 bit seconds)
 * Returns 0: if error
	   1: if OK, and value stored in value
 */
int duration_to_utime(char *str, utime_t *value)
{
   int i, mod_len;
   double val, total = 0.0;
   char mod_str[20];
   char num_str[50];
   /*
    * The "n" = mins and months appears before minutes so that m maps
    *   to months. These "kludges" make it compatible with pre 1.31
    *	Baculas.
    */
   static const char *mod[] = {"n", "seconds", "months", "minutes",
                  "hours", "days", "weeks",   "quarters",   "years", NULL};
   static const int32_t mult[] = {60,	1, 60*60*24*30, 60,
		  60*60, 60*60*24, 60*60*24*7, 60*60*24*91, 60*60*24*365};

   while (*str) {
      if (!get_modifier(str, num_str, sizeof(num_str), mod_str, sizeof(mod_str))) {
	 return 0;
      }
      /* Now find the multiplier corresponding to the modifier */
      mod_len = strlen(mod_str);
      if (mod_len == 0) {
	 i = 1; 			 /* assume seconds */
      } else {
	 for (i=0; mod[i]; i++) {
	    if (strncasecmp(mod_str, mod[i], mod_len) == 0) {
	       break;
	    }
	 }
	 if (mod[i] == NULL) {
	    i = 1;			 /* no modifier, assume secs */
	 }
      }
      Dmsg2(900, "str=%s: mult=%d\n", num_str, mult[i]);
      errno = 0;
      val = strtod(num_str, NULL);
      if (errno != 0 || val < 0) {
	 return 0;
      }
      total += val * mult[i];
   }
   *value = (utime_t)total;
   return 1;
}

/*
 * Edit a utime "duration" into ASCII
 */
char *edit_utime(utime_t val, char *buf, int buf_len)
{
   char mybuf[200];
   static const int32_t mult[] = {60*60*24*365, 60*60*24*30, 60*60*24, 60*60, 60};
   static const char *mod[]  = {"year",  "month",  "day", "hour", "min"};
   int i;
   uint32_t times;

   *buf = 0;
   for (i=0; i<5; i++) {
      times = (uint32_t)(val / mult[i]);
      if (times > 0) {
	 val = val - (utime_t)times * mult[i];
         bsnprintf(mybuf, sizeof(mybuf), "%d %s%s ", times, mod[i], times>1?"s":"");
	 bstrncat(buf, mybuf, buf_len);
      }
   }
   if (val == 0 && strlen(buf) == 0) {
      bstrncat(buf, "0 secs", buf_len);
   } else if (val != 0) {
      bsnprintf(mybuf, sizeof(mybuf), "%d sec%s", (uint32_t)val, val>1?"s":"");
      bstrncat(buf, mybuf, buf_len);
   }
   return buf;
}

/*
 * Convert a size in bytes to uint64_t
 * Returns 0: if error
	   1: if OK, and value stored in value
 */
int size_to_uint64(char *str, int str_len, uint64_t *value)
{
   int i, mod_len;
   double val;
   char mod_str[20];
   char num_str[50];
   static const char *mod[]  = {"*", "k", "kb", "m", "mb",  "g", "gb",  NULL}; /* first item * not used */
   const int64_t mult[] = {1,		  /* byte */
			   1024,	  /* kilobyte */
			   1000,	  /* kb kilobyte */
			   1048576,	  /* megabyte */
			   1000000,	  /* mb megabyte */
			   1073741824,	  /* gigabyte */
			   1000000000};   /* gb gigabyte */

   if (!get_modifier(str, num_str, sizeof(num_str), mod_str, sizeof(mod_str))) {
      return 0;
   }
   /* Now find the multiplier corresponding to the modifier */
   mod_len = strlen(mod_str);
   for (i=0; mod[i]; i++) {
      if (strncasecmp(mod_str, mod[i], mod_len) == 0) {
	 break;
      }
   }
   if (mod[i] == NULL) {
      i = 0;			      /* no modifier found, assume 1 */
   }
   Dmsg2(900, "str=%s: mult=%d\n", str, mult[i]);
   errno = 0;
   val = strtod(num_str, NULL);
   if (errno != 0 || val < 0) {
      return 0;
   }
  *value = (utime_t)(val * mult[i]);
   return 1;
}

/*
 * Check if specified string is a number or not.
 *  Taken from SQLite, cool, thanks.
 */
bool is_a_number(const char *n)
{
   bool digit_seen = false;

   if( *n == '-' || *n == '+' ) {
      n++;
   }
   while (B_ISDIGIT(*n)) {
      digit_seen = true;
      n++;
   }
   if (digit_seen && *n == '.') {
      n++;
      while (B_ISDIGIT(*n)) { n++; }
   }
   if (digit_seen && (*n == 'e' || *n == 'E')
       && (B_ISDIGIT(n[1]) || ((n[1]=='-' || n[1] == '+') && B_ISDIGIT(n[2])))) {
      n += 2;			      /* skip e- or e+ or e digit */
      while (B_ISDIGIT(*n)) { n++; }
   }
   return digit_seen && *n==0;
}

/*
 * Check if the specified string is an integer
 */
bool is_an_integer(const char *n)
{
   bool digit_seen = false;
   while (B_ISDIGIT(*n)) {
      digit_seen = true;
      n++;
   }
   return digit_seen && *n==0;
}

/*
 * Check if Bacula Resoure Name is valid
 */
/*
 * Check if the Volume name has legal characters
 * If ua is non-NULL send the message
 */
bool is_name_valid(char *name, POOLMEM **msg)
{
   int len;
   char *p;
   /* Special characters to accept */
   const char *accept = ":.-_ ";

   /* Restrict the characters permitted in the Volume name */
   for (p=name; *p; p++) {
      if (B_ISALPHA(*p) || B_ISDIGIT(*p) || strchr(accept, (int)(*p))) {
	 continue;
      }
      if (msg) {
         Mmsg(msg, _("Illegal character \"%c\" in name.\n"), *p);
      }
      return false;
   }
   len = strlen(name);
   if (len >= MAX_NAME_LENGTH) {
      if (msg) {
         Mmsg(msg, _("Name too long.\n"));
      }
      return false;
   }
   if (len == 0) {
      if (msg) {
         Mmsg(msg,  _("Volume name must be at least one character long.\n"));
      }
      return false;
   }
   return true;
}



/*
 * Add commas to a string, which is presumably
 * a number.
 */
char *add_commas(char *val, char *buf)
{
   int len, nc;
   char *p, *q;
   int i;

   if (val != buf) {
      strcpy(buf, val);
   }
   len = strlen(buf);
   if (len < 1) {
      len = 1;
   }
   nc = (len - 1) / 3;
   p = buf+len;
   q = p + nc;
   *q-- = *p--;
   for ( ; nc; nc--) {
      for (i=0; i < 3; i++) {
	  *q-- = *p--;
      }
      *q-- = ',';
   }
   return buf;
}

#ifdef TEST_PROGRAM
void d_msg(const char*, int, int, const char*, ...)
{}
int main(int argc, char *argv[])
{
   char *str[] = {"3", "3n", "3 hours", "3.5 day", "3 week", "3 m", "3 q", "3 years"};
   utime_t val;
   char buf[100];
   char outval[100];

   for (int i=0; i<8; i++) {
      strcpy(buf, str[i]);
      if (!duration_to_utime(buf, &val)) {
         printf("Error return from duration_to_utime for in=%s\n", str[i]);
	 continue;
      }
      edit_utime(val, outval);
      printf("in=%s val=%" lld " outval=%s\n", str[i], val, outval);
   }
}
#endif
