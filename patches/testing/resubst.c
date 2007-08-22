#include "bacula.h"
#include "bregex.h"

/* s/toto(.)/titi$1/ 
 * toto est beau => titi est beau
 *
 */

/* re_match()
 * compute_dest_len()
 * check_pool_size()
 * edit()
 */

typedef struct {
   char *subst;
   char *motif;
   int nmatch;
   regex_t preg;
} breg_t ;

breg_t *breg_new(char *regexp)
{
   Dmsg0(500, "breg: creating new breg_t object\n");
   RUNSCRIPT *cmd = (RUNSCRIPT *)malloc(sizeof(RUNSCRIPT));
   memset(cmd, 0, sizeof(RUNSCRIPT));
   cmd->reset_default();
   
   return cmd;

}


int compute_dest_len(char *fname, char *subst, 
		     regmatch_t *pmatch, int nmatch)
{
   int len=0;
   char *p;
   int no;

   if (!fname || !subst || !pmatch || !nmatch) {
      return 0;
   }

   /* match failed ? */
   if (pmatch[0].rm_so < 0) {
      return 0;
   }

   for (p = subst++; *p ; p = subst++) {
      /* match $1 \1 back references */
      if ((*p == '$' || *p == '\\') && ('0' <= *subst && *subst <= '9')) {
	 no = *subst++ - '0';

	 /* we check if the back reference exists */
	 if (no < nmatch && pmatch[no].rm_so >= 0 && pmatch[no].rm_eo >= 0) { 
	    len += pmatch[no].rm_eo - pmatch[no].rm_so;
	 } else {
	    return 0; /* back reference missing or reference number > nmatch */
	 }
      } else {
	 len++;
      }
   }

   /* $0 is replaced by subst */
   len -= pmatch[0].rm_eo - pmatch[0].rm_so;
   len += strlen(fname) + 1;

   return len;
}

/* /toto/titi/ 
 * preg
 * subst
 */
bool extract_regexp(char *motif, regex_t *preg, POOLMEM **subst)
{
   if (!motif || !preg || !subst) {
      return false;
   }
   /* extract 1st part */
   POOLMEM *dest = bstrdup(motif);
   char sep = motif[0];
   char *search = motif + 1;
   char *replace;
   bool ok = false;
   bool found_motif = false;

   while (*search && !ok) {
      if (*search == sep && *dest == '\\') {
	 *dest++ = *++search;       /* we skip separator */ 

      } else if (*search == sep) {
	 *dest++ = '\0';
	 if (found_motif) {	/* already have found motif */
	    ok = true;
	 } else {
	    replace = dest;	/* get replaced string */
	    found_motif = true;
	 }
      } else {
	 *dest++ = *search++;
      }
   }
   *dest = '\0';		/* in case of */
   
   if (!ok || !found_motif) {
      /* bad regexp */
      free(dest);
      return false;
   }

   
  

   /* rechercher le 1er car sans \ devant */
   /* compiler la re dans preg */
   /* extraire le subst */
   /* verifier le nombre de reference */
}

/* dest is long enough */
char *edit_subst(char *fname, char *subst, regmatch_t *pmatch, char *dest)
{
   int i;
   char *p;
   int no;
   int len;

   /* il faut recopier fname dans dest
    *  on recopie le debut fname -> pmatch[0].rm_so
    */
   
   for (i = 0; i < pmatch[0].rm_so ; i++) {
      dest[i] = fname[i];
   }

   /* on recopie le motif de remplacement (avec tous les $x) */

   for (p = subst++; *p ; p = subst++) {
      /* match $1 \1 back references */
      if ((*p == '$' || *p == '\\') && ('0' <= *subst && *subst <= '9')) {
	 no = *subst++ - '0';

	 len = pmatch[no].rm_eo - pmatch[no].rm_so;
	 bstrncpy(dest + i, fname + pmatch[no].rm_so, len);
	 i += len;

      } else {
	 dest[i++] = *p;
      }
   }

   strcpy(dest + i, fname + pmatch[0].rm_eo);

   return dest;
}

/* return jcr->subst_fname or fname */
char *fname_subst(JCR *jcr, char *fname)
{
   /* in JCR */
   regex_t preg;
   char *pat="$";
   char *subst=".old";
   char *dest=NULL;

   int rc = regcomp(&preg, pat, REG_EXTENDED);
   if (rc != 0) {
      char prbuf[500];
      regerror(rc, &preg, prbuf, sizeof(prbuf));
      printf("Regex compile error: %s\n", prbuf);
      return fname;
   }

   const int nmatch = 30;
   regmatch_t pmatch[nmatch];
   rc = regexec(&preg, fname, nmatch, pmatch,  0);

   if (!rc) {
      char prbuf[500];
      regerror(rc, &preg, prbuf, sizeof(prbuf));
      printf("Regex error: %s\n", prbuf);
      return fname;
   }

   int len = compute_dest_len(fname, subst, 
			      pmatch, nmatch);

   if (len) {
      dest = (char *)malloc(len);
      edit_subst(fname, subst, pmatch, dest);
   }

   return dest;
} 
