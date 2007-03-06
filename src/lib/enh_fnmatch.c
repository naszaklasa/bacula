/* Copyright (C) 1991, 1992, 1993, 1996, 1997 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Modified version of fnmatch.c - Robert Nelson */

#include "bacula.h"
#include "enh_fnmatch.h"

# ifndef errno
extern int errno;
# endif

int
enh_fnmatch_sub(const char *pattern, const char *string, int patternlen, int flags)
{
  register const char *p = pattern, *n = string;
  register char c;

/* Note that this evaluates C many times.  */
# define FOLD(c) ((flags & FNM_CASEFOLD) && B_ISUPPER (c) ? tolower (c) : (c))

  while ((p - pattern) < patternlen)
    {
      c = *p++;
      c = FOLD (c);

      switch (c)
        {
        case '?':
          if (*n == '\0')
            return 0;
          else if ((flags & FNM_FILE_NAME) && IsPathSeparator(*n))
            return 0;
          else if ((flags & FNM_PERIOD) && *n == '.' &&
                   (n == string || ((flags & FNM_FILE_NAME) && IsPathSeparator(n[-1]))))
            return 0;
          break;

        case '\\':
          if (!(flags & FNM_NOESCAPE))
            {
              if ((p - pattern) >= patternlen)
                /* Trailing \ loses.  */
                return 0;

              c = *p++;
              c = FOLD(c);
            }
          if (FOLD (*n) != c)
            return 0;
          break;

        case '*':
          if ((flags & FNM_PERIOD) && *n == '.' &&
              (n == string || ((flags & FNM_FILE_NAME) && IsPathSeparator(n[-1]))))
            return FNM_NOMATCH;

          if ((p - pattern) >= patternlen)
              return patternlen;

          for (c = *p++; ((p - pattern) <= patternlen) && (c == '?' || c == '*'); c = *p++)
            {
              if ((flags & FNM_FILE_NAME) && IsPathSeparator(*n))
                /* A slash does not match a wildcard under FNM_FILE_NAME.  */
                return 0;
              else if (c == '?')
                {
                  /* A ? needs to match one character.  */
                  if (*n == '\0')
                    /* There isn't another character; no match.  */
                    return 0;
                  else
                    /* One character of the string is consumed in matching
                       this ? wildcard, so *??? won't match if there are
                       less than three characters.  */
                    ++n;
                }
            }

          if ((p - pattern) >= patternlen)
              return patternlen;

          {
            char c1 = (!(flags & FNM_NOESCAPE) && c == '\\') ? *p : c;
            c1 = FOLD (c1);
            for (--p; *n != '\0'; ++n)
              {
                if (c == '[' || c == '{' || FOLD((unsigned char)*n) == c1)
                  {
                    int len;

                    len = enh_fnmatch_sub(p, n, (int)(patternlen - (p - pattern)), flags & ~FNM_PERIOD);

                    if (len > 0 && n[len] == '\0')
                      return (int)(n - string + len);
                  }
                else
                  {
                    if ((flags & FNM_FILE_NAME) && IsPathSeparator(*n))
                      return 0;    /* A slash does not match a wildcard under FNM_FILE_NAME.  */
                  }

              }
            return 0;
          }

        case '{':
          {
            const char *pstart = p;

            while ((p - pattern) < patternlen)
              {
                c = *p++;

                if (!(flags & FNM_NOESCAPE) && c == '\\')
                  {
                    if ((p - pattern) >= patternlen)
                      return 0;

                    ++p;
                    continue;
                  }
              
                if (c == ',' || c == '}')
                  {
                    int matchlen;
                  
                    matchlen = enh_fnmatch_sub(pstart, n, (int)(p - pstart - 1), flags & ~FNM_PERIOD);

                    if (matchlen > 0)
                      {
                        n += matchlen - 1;
                        while (c != '}')
                          {
                            if (!(flags & FNM_NOESCAPE) && c == '\\')
                              {
                                if ((p - pattern) >= patternlen)
                                  return 0;

                                ++p;
                              }

                            if ((p - pattern) >= patternlen)
                              return 0;

                            c = *p++;
                          }
                        break;
                      }

                    if (c == '}')
                      return 0;

                    pstart = p;
                  }
              }
            break;
          }

        case '[':
          {
            /* Nonzero if the sense of the character class is inverted.  */
            register int nnot;

            if (*n == '\0')
              return 0;

            if ((flags & FNM_PERIOD) && *n == '.' &&
                (n == string || ((flags & FNM_FILE_NAME) && IsPathSeparator(n[-1]))))
              return 0;

            nnot = (*p == '!' || *p == '^');
            if (nnot)
              ++p;

            if ((p - pattern) >= patternlen)
              /* [ (unterminated) loses.  */
              return 0;

            c = *p++;

            for (;;)
              {
                register char cstart, cend;

                cstart = cend = FOLD (c);

                if ((p - pattern) >= patternlen)
                  /* [ (unterminated) loses.  */
                  return 0;

                c = *p++;
                c = FOLD (c);

                if ((flags & FNM_FILE_NAME) && IsPathSeparator(c))
                  /* [/] can never match.  */
                  return 0;

                if (c == '-' && *p != ']')
                  {
                    if ((p - pattern) >= patternlen)
                      return 0;

                    cend = *p++;

                    cend = FOLD (cend);

                    if ((p - pattern) >= patternlen)
                      return 0;

                    c = *p++;
                  }

                if (FOLD (*n) >= cstart && FOLD (*n) <= cend)
                  goto matched;

                if (c == ']')
                  break;
              }
            if (!nnot)
              return 0;
            break;

          matched:;
            /* Skip the rest of the [...] that already matched.  */
            while (c != ']')
              {
                if ((p - pattern) >= patternlen)
                  return 0;

                c = *p++;
              }
            if (nnot)
              return 0;
          }
          break;

        default:
          if (c != FOLD (*n))
            return 0;
          break;
        }

      ++n;
    }

    return (int)(n - string);

# undef FOLD
}

/* Match STRING against the filename pattern PATTERN, returning number of characters
   in STRING that were matched if all of PATTERN matches, nonzero if not.  */
int
enh_fnmatch(const char *pattern, const char *string, int flags)
{
  int matchlen;

  matchlen = enh_fnmatch_sub(pattern, string, (int)strlen(pattern), flags);

  if (matchlen == 0)
    return FNM_NOMATCH;

  if (string[matchlen] == '\0')
    return 0;

  if ((flags & FNM_LEADING_DIR) && IsPathSeparator(string[matchlen]))
    /* The FNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */
    return 0;

  return FNM_NOMATCH;
}
