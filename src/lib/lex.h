/*
 *   lex.h
 *
 *    Lexical scanning of configuration files, used by parsers.
 *
 *   Kern Sibbald, MM
 *
 *   Version $Id: lex.h,v 1.9.4.1 2005/02/14 10:02:26 kerns Exp $
 *
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald

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

#ifndef _LEX_H
#define _LEX_H

/* Lex get_char() return values */
#define L_EOF                         (-1)
#define L_EOL                         (-2)

/* Internal tokens */
#define T_NONE                        100

/* Tokens returned by get_token() */
#define T_EOF                         101
#define T_NUMBER                      102
#define T_IPADDR                      103
#define T_IDENTIFIER                  104
#define T_UNQUOTED_STRING             105
#define T_QUOTED_STRING               106
#define T_BOB                         108  /* begin block */
#define T_EOB                         109  /* end of block */
#define T_EQUALS                      110
#define T_COMMA                       111
#define T_EOL                         112
#define T_ERROR                       200
/*
 * The following will be returned only if
 * the appropriate expect flag has been set
 */
#define T_SKIP_EOL                    113  /* scan through EOLs */
#define T_PINT32                      114  /* positive integer */
#define T_PINT32_RANGE                115  /* positive integer range */
#define T_INT32                       116  /* integer */
#define T_INT64                       117  /* 64 bit integer */
#define T_NAME                        118  /* name max 128 chars */
#define T_STRING                      119  /* string */

#define T_ALL                           0  /* no expectations */

/* Lexical state */
enum lex_state {
   lex_none,
   lex_comment,
   lex_number,
   lex_ip_addr,
   lex_identifier,
   lex_string,
   lex_quoted_string,
   lex_include
};

/* Lex scan options */
#define LOPT_NO_IDENT            0x1  /* No Identifiers -- use string */
#define LOPT_STRING              0x2  /* Force scan for string */

/* Lexical context */
typedef struct s_lex_context {
   struct s_lex_context *next;        /* pointer to next lexical context */
   int options;                       /* scan options */
   char *fname;                       /* filename */
   FILE *fd;                          /* file descriptor */
   char line[MAXSTRING];              /* input line */
   char str[MAXSTRING];               /* string being scanned */
   int str_len;                       /* length of string */
   int line_no;                       /* file line number */
   int col_no;                        /* char position on line */
   int begin_line_no;                 /* line no of beginning of string */
   enum lex_state state;              /* lex_state variable */
   int ch;                            /* last char/L_VAL returned by get_char */
   int token;
   uint32_t pint32_val;
   uint32_t pint32_val2;
   int32_t int32_val;
   int64_t int64_val;
   void (*scan_error)(const char *file, int line, struct s_lex_context *lc, const char *msg, ...);
   void *caller_ctx;                  /* caller private data */
} LEX;

typedef void (LEX_ERROR_HANDLER)(const char *file, int line, LEX *lc, const char *msg, ...);

/* Lexical scanning errors in parsing conf files */
#define scan_err0(lc, msg) lc->scan_error(__FILE__, __LINE__, lc, msg)
#define scan_err1(lc, msg, a1) lc->scan_error(__FILE__, __LINE__, lc, msg, a1)
#define scan_err2(lc, msg, a1, a2) lc->scan_error(__FILE__, __LINE__, lc, msg, a1, a2)
#define scan_err3(lc, msg, a1, a2, a3) lc->scan_error(__FILE__, __LINE__, lc, msg, a1, a2, a3)
#define scan_err4(lc, msg, a1, a2, a3, a4) lc->scan_error(__FILE__, __LINE__, lc, msg, a1, a2, a3, a4)
#define scan_err5(lc, msg, a1, a2, a3, a4, a5) lc->scan_error(__FILE__, __LINE__, lc, msg, a1, a2, a3, a4, a5)
#define scan_err6(lc, msg, a1, a2, a3, a4, a5, a6) lc->scan_error(__FILE__, __LINE__, lc, msg, a1, a2, a3, a4, a5, a6)

void scan_to_eol(LEX *lc);
int scan_to_next_not_eol(LEX * lc);

#endif /* _LEX_H */
