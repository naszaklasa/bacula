/*
 * Lexical scanner for Bacula configuration file
 *
 *   Kern Sibbald, 2000
 *
 *   Version $Id: lex.c,v 1.38 2005/08/10 16:35:19 nboichat Exp $
 *
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
#include "lex.h"

extern int debug_level;

/*
 * Scan to "logical" end of line. I.e. end of line,
 *   or semicolon, but stop on T_EOB (same as end of
 *   line except it is not eaten).
 */
void scan_to_eol(LEX *lc)
{
   int token;
   Dmsg0(2000, "start scan to eof\n");
   while ((token = lex_get_token(lc, T_ALL)) != T_EOL) {
      if (token == T_EOB) {
         lex_unget_char(lc);
         return;
      }
   }
}

/*
 * Get next token, but skip EOL
 */
int scan_to_next_not_eol(LEX * lc)
{
   int token;
   do {
      token = lex_get_token(lc, T_ALL);
   } while (token == T_EOL);
   return token;
}

/*
 * Format a scanner error message
 */
static void s_err(const char *file, int line, LEX *lc, const char *msg, ...)
{
   va_list arg_ptr;
   char buf[MAXSTRING];
   char more[MAXSTRING];

   va_start(arg_ptr, msg);
   bvsnprintf(buf, sizeof(buf), msg, arg_ptr);
   va_end(arg_ptr);

   if (lc->line_no > lc->begin_line_no) {
      bsnprintf(more, sizeof(more),
                _("Problem probably begins at line %d.\n"), lc->begin_line_no);
   } else {
      more[0] = 0;
   }  
   if (lc->line_no > 0) {
      e_msg(file, line, M_ERROR_TERM, 0, _("Config error: %s\n"
"            : line %d, col %d of file %s\n%s\n%s"),
         buf, lc->line_no, lc->col_no, lc->fname, lc->line, more);
   } else {
      e_msg(file, line, M_ERROR_TERM, 0, _("Config error: %s\n"), buf);
   }
}

void lex_set_default_error_handler(LEX *lf)
{
   lf->scan_error = s_err;
}


/*
 * Free the current file, and retrieve the contents
 * of the previous packet if any.
 */
LEX *lex_close_file(LEX *lf)
{
   LEX *of;

   Dmsg1(2000, "Close lex file: %s\n", lf->fname);
   if (lf == NULL) {
      Emsg0(M_ABORT, 0, _("Close of NULL file\n"));
   }
   of = lf->next;
   fclose(lf->fd);
   Dmsg1(2000, "Close cfg file %s\n", lf->fname);
   free(lf->fname);
   if (of) {
      of->options = lf->options;      /* preserve options */
      memcpy(lf, of, sizeof(LEX));
      Dmsg1(2000, "Restart scan of cfg file %s\n", of->fname);
   } else {
      of = lf;
      lf = NULL;
   }
   free(of);
   return lf;
}

/*
 * Open a new configuration file. We push the
 * state of the current file (lf) so that we
 * can do includes.  This is a bit of a hammer.
 * Instead of passing back the pointer to the
 * new packet, I simply replace the contents
 * of the caller's packet with the new packet,
 * and link the contents of the old packet into
 * the next field.
 *
 */
LEX *lex_open_file(LEX *lf, const char *filename, LEX_ERROR_HANDLER *scan_error)

{
   LEX *nf;
   FILE *fd;
   char *fname = bstrdup(filename);


   if ((fd = fopen(fname, "r")) == NULL) {
      return NULL;
   }
   Dmsg1(400, "Open config file: %s\n", fname);
   nf = (LEX *)malloc(sizeof(LEX));
   if (lf) {
      memcpy(nf, lf, sizeof(LEX));
      memset(lf, 0, sizeof(LEX));
      lf->next = nf;                  /* if have lf, push it behind new one */
      lf->options = nf->options;      /* preserve user options */
   } else {
      lf = nf;                        /* start new packet */
      memset(lf, 0, sizeof(LEX));
   }
   if (scan_error) {
      lf->scan_error = scan_error;
   } else {
      lex_set_default_error_handler(lf);
   }
   lf->fd = fd;
   lf->fname = fname;
   lf->state = lex_none;
   lf->ch = L_EOL;
   Dmsg1(2000, "Return lex=%x\n", lf);
   return lf;
}

/*
 * Get the next character from the input.
 *  Returns the character or
 *    L_EOF if end of file
 *    L_EOL if end of line
 */
int lex_get_char(LEX *lf)
{
   if (lf->ch == L_EOF) {
      Emsg0(M_ABORT, 0, _("get_char: called after EOF\n"));
   }
   if (lf->ch == L_EOL) {
      if (bfgets(lf->line, MAXSTRING, lf->fd) == NULL) {
         lf->ch = L_EOF;
         if (lf->next) {
            lex_close_file(lf);
         }
         return lf->ch;
      }
      lf->line_no++;
      lf->col_no = 0;
      Dmsg2(1000, "fget line=%d %s", lf->line_no, lf->line);
   }
   lf->ch = (uint8_t)lf->line[lf->col_no];
   if (lf->ch == 0) {
      lf->ch = L_EOL;
   } else {
      lf->col_no++;
   }
   Dmsg2(2000, "lex_get_char: %c %d\n", lf->ch, lf->ch);
   return lf->ch;
}

void lex_unget_char(LEX *lf)
{
   lf->col_no--;
   if (lf->ch == L_EOL)
      lf->ch = 0;
}


/*
 * Add a character to the current string
 */
static void add_str(LEX *lf, int ch)
{
   if (lf->str_len >= MAXSTRING-3) {
      Emsg3(M_ERROR_TERM, 0, _(
           _("Config token too long, file: %s, line %d, begins at line %d\n")),
             lf->fname, lf->line_no, lf->begin_line_no);
   }
   lf->str[lf->str_len++] = ch;
   lf->str[lf->str_len] = 0;
}

/*
 * Begin the string
 */
static void begin_str(LEX *lf, int ch)
{
   lf->str_len = 0;
   lf->str[0] = 0;
   if (ch != 0) {
      add_str(lf, ch);
   }
   lf->begin_line_no = lf->line_no;   /* save start string line no */
}

#ifdef DEBUG
static const char *lex_state_to_str(int state)
{
   switch (state) {
   case lex_none:          return _("none");
   case lex_comment:       return _("comment");
   case lex_number:        return _("number");
   case lex_ip_addr:       return _("ip_addr");
   case lex_identifier:    return _("identifier");
   case lex_string:        return _("string");
   case lex_quoted_string: return _("quoted_string");
   default:                return "??????";
   }
}
#endif

/*
 * Convert a lex token to a string
 * used for debug/error printing.
 */
const char *lex_tok_to_str(int token)
{
   switch(token) {
   case L_EOF:             return "L_EOF";
   case L_EOL:             return "L_EOL";
   case T_NONE:            return "T_NONE";
   case T_NUMBER:          return "T_NUMBER";
   case T_IPADDR:          return "T_IPADDR";
   case T_IDENTIFIER:      return "T_IDENTIFIER";
   case T_UNQUOTED_STRING: return "T_UNQUOTED_STRING";
   case T_QUOTED_STRING:   return "T_QUOTED_STRING";
   case T_BOB:             return "T_BOB";
   case T_EOB:             return "T_EOB";
   case T_EQUALS:          return "T_EQUALS";
   case T_ERROR:           return "T_ERROR";
   case T_EOF:             return "T_EOF";
   case T_COMMA:           return "T_COMMA";
   case T_EOL:             return "T_EOL";
   default:                return "??????";
   }
}

static uint32_t scan_pint(LEX *lf, char *str)
{
   int64_t val = 0;
   if (!is_a_number(str)) {
      scan_err1(lf, _("expected a positive integer number, got: %s"), str);
      /* NOT REACHED */
   } else {
      errno = 0;
      val = str_to_int64(str);
      if (errno != 0 || val < 0) {
         scan_err1(lf, _("expected a postive integer number, got: %s"), str);
         /* NOT REACHED */
      }
   }
   return (uint32_t)val;
}

/*
 *
 * Get the next token from the input
 *
 */
int
lex_get_token(LEX *lf, int expect)
{
   int ch;
   int token = T_NONE;
   bool esc_next = false;

   Dmsg0(2000, "enter lex_get_token\n");
   while (token == T_NONE) {
      ch = lex_get_char(lf);
      switch (lf->state) {
      case lex_none:
         Dmsg2(2000, "Lex state lex_none ch=%d,%x\n", ch, ch);
         if (B_ISSPACE(ch))
            break;
         if (B_ISALPHA(ch)) {
            if (lf->options & LOPT_NO_IDENT || lf->options & LOPT_STRING) {
               lf->state = lex_string;
            } else {
               lf->state = lex_identifier;
            }
            begin_str(lf, ch);
            break;
         }
         if (B_ISDIGIT(ch)) {
            if (lf->options & LOPT_STRING) {
               lf->state = lex_string;
            } else {
               lf->state = lex_number;
            }
            begin_str(lf, ch);
            break;
         }
         Dmsg0(2000, "Enter lex_none switch\n");
         switch (ch) {
         case L_EOF:
            token = T_EOF;
            Dmsg0(2000, "got L_EOF set token=T_EOF\n");
            break;
         case '#':
            lf->state = lex_comment;
            break;
         case '{':
            token = T_BOB;
            begin_str(lf, ch);
            break;
         case '}':
            token = T_EOB;
            begin_str(lf, ch);
            break;
         case '"':
            lf->state = lex_quoted_string;
            begin_str(lf, 0);
            break;
         case '=':
            token = T_EQUALS;
            begin_str(lf, ch);
            break;
         case ',':
            token = T_COMMA;
            begin_str(lf, ch);
            break;
         case ';':
            if (expect != T_SKIP_EOL) {
               token = T_EOL;      /* treat ; like EOL */
            }
            break;
         case L_EOL:
            Dmsg0(2000, "got L_EOL set token=T_EOL\n");
            if (expect != T_SKIP_EOL) {
               token = T_EOL;
            }
            break;
         case '@':
            lf->state = lex_include;
            begin_str(lf, 0);
            break;
         default:
            lf->state = lex_string;
            begin_str(lf, ch);
            break;
         }
         break;
      case lex_comment:
         Dmsg1(2000, "Lex state lex_comment ch=%x\n", ch);
         if (ch == L_EOL) {
            lf->state = lex_none;
            if (expect != T_SKIP_EOL) {
               token = T_EOL;
            }
         } else if (ch == L_EOF) {
            token = T_ERROR;
         }
         break;
      case lex_number:
         Dmsg2(2000, "Lex state lex_number ch=%x %c\n", ch, ch);
         if (ch == L_EOF) {
            token = T_ERROR;
            break;
         }
         /* Might want to allow trailing specifications here */
         if (B_ISDIGIT(ch)) {
            add_str(lf, ch);
            break;
         }

         /* A valid number can be terminated by the following */
         if (B_ISSPACE(ch) || ch == L_EOL || ch == ',' || ch == ';') {
            token = T_NUMBER;
            lf->state = lex_none;
         } else {
            lf->state = lex_string;
         }
         lex_unget_char(lf);
         break;
      case lex_ip_addr:
         if (ch == L_EOF) {
            token = T_ERROR;
            break;
         }
         Dmsg1(2000, "Lex state lex_ip_addr ch=%x\n", ch);
         break;
      case lex_string:
         Dmsg1(2000, "Lex state lex_string ch=%x\n", ch);
         if (ch == L_EOF) {
            token = T_ERROR;
            break;
         }
         if (ch == '\n' || ch == L_EOL || ch == '=' || ch == '}' || ch == '{' ||
             ch == '\r' || ch == ';' || ch == ',' || ch == '#' || (B_ISSPACE(ch)) ) {
            lex_unget_char(lf);
            token = T_UNQUOTED_STRING;
            lf->state = lex_none;
            break;
         }
         add_str(lf, ch);
         break;
      case lex_identifier:
         Dmsg2(2000, "Lex state lex_identifier ch=%x %c\n", ch, ch);
         if (B_ISALPHA(ch)) {
            add_str(lf, ch);
            break;
         } else if (B_ISSPACE(ch)) {
            break;
         } else if (ch == '\n' || ch == L_EOL || ch == '=' || ch == '}' || ch == '{' ||
                    ch == '\r' || ch == ';' || ch == ','   || ch == '"' || ch == '#') {
            lex_unget_char(lf);
            token = T_IDENTIFIER;
            lf->state = lex_none;
            break;
         } else if (ch == L_EOF) {
            token = T_ERROR;
            lf->state = lex_none;
            begin_str(lf, ch);
            break;
         }
         /* Some non-alpha character => string */
         lf->state = lex_string;
         add_str(lf, ch);
         break;
      case lex_quoted_string:
         Dmsg2(2000, "Lex state lex_quoted_string ch=%x %c\n", ch, ch);
         if (ch == L_EOF) {
            token = T_ERROR;
            break;
         }
         if (ch == L_EOL) {
            esc_next = false;
            break;
         }
         if (esc_next) {
            add_str(lf, ch);
            esc_next = false;
            break;
         }
         if (ch == '\\') {
            esc_next = true;
            break;
         }
         if (ch == '"') {
            token = T_QUOTED_STRING;
            lf->state = lex_none;
            break;
         }
         add_str(lf, ch);
         break;
      case lex_include:            /* scanning a filename */
         if (ch == L_EOF) {
            token = T_ERROR;
            break;
         }
         if (B_ISSPACE(ch) || ch == '\n' || ch == L_EOL || ch == '}' || ch == '{' ||
             ch == ';' || ch == ','   || ch == '"' || ch == '#') {
            /* Keep the original LEX so we can print an error if the included file can't be opened. */
            LEX* lfori = lf;
            
            lf->state = lex_none;
            lf = lex_open_file(lf, lf->str, NULL);
            if (lf == NULL) {
               berrno be;
               scan_err2(lfori, _("Cannot open included config file %s: %s\n"),
                  lfori->str, be.strerror());
               return T_ERROR;
            }
            break;
         }
         add_str(lf, ch);
         break;
      }
      Dmsg4(2000, "ch=%d state=%s token=%s %c\n", ch, lex_state_to_str(lf->state),
        lex_tok_to_str(token), ch);
   }
   Dmsg2(2000, "lex returning: line %d token: %s\n", lf->line_no, lex_tok_to_str(token));
   lf->token = token;

   /*
    * Here is where we check to see if the user has set certain
    *  expectations (e.g. 32 bit integer). If so, we do type checking
    *  and possible additional scanning (e.g. for range).
    */
   switch (expect) {
   case T_PINT32:
      lf->pint32_val = scan_pint(lf, lf->str);
      lf->pint32_val2 = lf->pint32_val;
      token = T_PINT32;
      break;

   case T_PINT32_RANGE:
      if (token == T_NUMBER) {
         lf->pint32_val = scan_pint(lf, lf->str);
         lf->pint32_val2 = lf->pint32_val;
         token = T_PINT32;
      } else {
         char *p = strchr(lf->str, '-');
         if (!p) {
            scan_err2(lf, _("expected an integer or a range, got %s: %s"),
               lex_tok_to_str(token), lf->str);
            token = T_ERROR;
            break;
         }
         *p++ = 0;                       /* terminate first half of range */
         lf->pint32_val  = scan_pint(lf, lf->str);
         lf->pint32_val2 = scan_pint(lf, p);
         token = T_PINT32_RANGE;
      }
      break;

   case T_INT32:
      if (token != T_NUMBER || !is_a_number(lf->str)) {
         scan_err2(lf, _("expected an integer number, got %s: %s"),
               lex_tok_to_str(token), lf->str);
         token = T_ERROR;
         break;
      }
      errno = 0;
      lf->int32_val = (int32_t)str_to_int64(lf->str);
      if (errno != 0) {
         scan_err2(lf, _("expected an integer number, got %s: %s"),
               lex_tok_to_str(token), lf->str);
         token = T_ERROR;
      } else {
         token = T_INT32;
      }
      break;

   case T_INT64:
      Dmsg2(2000, "int64=:%s: %f\n", lf->str, strtod(lf->str, NULL));
      if (token != T_NUMBER || !is_a_number(lf->str)) {
         scan_err2(lf, _("expected an integer number, got %s: %s"),
               lex_tok_to_str(token), lf->str);
         token = T_ERROR;
         break;
      }
      errno = 0;
      lf->int64_val = str_to_int64(lf->str);
      if (errno != 0) {
         scan_err2(lf, _("expected an integer number, got %s: %s"),
               lex_tok_to_str(token), lf->str);
         token = T_ERROR;
      } else {
         token = T_INT64;
      }
      break;

   case T_NAME:
      if (token != T_IDENTIFIER && token != T_UNQUOTED_STRING && token != T_QUOTED_STRING) {
         scan_err2(lf, _("expected a name, got %s: %s"),
               lex_tok_to_str(token), lf->str);
         token = T_ERROR;
      } else if (lf->str_len > MAX_RES_NAME_LENGTH) {
         scan_err3(lf, _("name %s length %d too long, max is %d\n"), lf->str,
            lf->str_len, MAX_RES_NAME_LENGTH);
         token = T_ERROR;
      }
      break;

   case T_STRING:
      if (token != T_IDENTIFIER && token != T_UNQUOTED_STRING && token != T_QUOTED_STRING) {
         scan_err2(lf, _("expected a string, got %s: %s"),
               lex_tok_to_str(token), lf->str);
         token = T_ERROR;
      } else {
         token = T_STRING;
      }
      break;


   default:
      break;                          /* no expectation given */
   }
   lf->token = token;                 /* set possible new token */
   return token;
}
