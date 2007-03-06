/*
 *   attr.c  Unpack an Attribute record returned from the tape
 * 
 *    Kern Sibbald, June MMIII	(code pulled from filed/restore.c and updated)
 *
 *   Version $Id: attr.c,v 1.11 2004/11/15 22:43:33 kerns Exp $
 */

/*
   Copyright (C) 2003-2004 Kern Sibbald and John Walker

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
#include "jcr.h"

extern const int win32_client;

ATTR *new_attr()
{
   ATTR *attr = (ATTR *)malloc(sizeof(ATTR));
   memset(attr, 0, sizeof(ATTR));
   attr->ofname = get_pool_memory(PM_FNAME);
   attr->olname = get_pool_memory(PM_FNAME);
   attr->attrEx = get_pool_memory(PM_FNAME);
   return attr;
}

void free_attr(ATTR *attr)
{
   free_pool_memory(attr->olname);
   free_pool_memory(attr->ofname);
   free_pool_memory(attr->attrEx);
   free(attr);
}

int unpack_attributes_record(JCR *jcr, int32_t stream, char *rec, ATTR *attr)
{
   char *p;
   /*		   
    * An Attributes record consists of:
    *	 File_index
    *	 Type	(FT_types)
    *	 Filename
    *	 Attributes
    *	 Link name (if file linked i.e. FT_LNK)
    *	 Extended attributes (Win32)
    *  plus optional values determined by AR_ flags in upper bits of Type
    *	 Data_stream
    *
    */
   attr->stream = stream;
   Dmsg1(100, "Attr: %s\n", rec);
   if (sscanf(rec, "%d %d", &attr->file_index, &attr->type) != 2) {
      Jmsg(jcr, M_FATAL, 0, _("Error scanning attributes: %s\n"), rec);
      Dmsg1(100, "\nError scanning attributes. %s\n", rec);
      return 0;
   }
   Dmsg2(100, "Got Attr: FilInx=%d type=%d\n", attr->file_index, attr->type);
   if (attr->type & AR_DATA_STREAM) {
      attr->data_stream = 1;
   } else {
      attr->data_stream = 0;
   }
   attr->type &= FT_MASK;	      /* keep only type bits */
   p = rec;
   while (*p++ != ' ')               /* skip record file index */
      { }
   while (*p++ != ' ')               /* skip type */
      { }
   
   attr->fname = p;		      /* set filname position */
   while (*p++ != 0)		      /* skip filename */
      { }
   attr->attr = p;		      /* set attributes position */
   while (*p++ != 0)		      /* skip attributes */
      { }
   attr->lname = p;		      /* set link position */
   while (*p++ != 0)		      /* skip link */
      { }
   pm_strcpy(attr->attrEx, p);	      /* copy extended attributes, if any */

   if (attr->data_stream) {
      int64_t val;
      while (*p++ != 0) 	      /* skip extended attributes */
	 { }
      from_base64(&val, p);
      attr->data_stream = (int32_t)val;
   }
   Dmsg7(200, "unpack_attr FI=%d Type=%d fname=%s attr=%s lname=%s attrEx=%s ds=%d\n",
      attr->file_index, attr->type, attr->fname, attr->attr, attr->lname,
      attr->attrEx, attr->data_stream);
   *attr->ofname = 0;
   *attr->olname = 0;
   return 1;
}

/*
 * Build attr->ofname from attr->fname and
 *	 attr->olname from attr->olname 
 */
void build_attr_output_fnames(JCR *jcr, ATTR *attr)
{
   /*
    * Prepend the where directory so that the
    * files are put where the user wants.
    *
    * We do a little jig here to handle Win32 files with
    *	a drive letter -- we simply change the drive
    *	from, for example, c: to c/ for
    *	every filename if a prefix is supplied.
    *	  
    */
   if (jcr->where[0] == 0) {
      pm_strcpy(attr->ofname, attr->fname);
      pm_strcpy(attr->olname, attr->lname);
   } else {
      const char *fn;
      int wherelen = strlen(jcr->where);
      pm_strcpy(attr->ofname, jcr->where);  /* copy prefix */
      if (win32_client && attr->fname[1] == ':') {
         attr->fname[1] = '/';     /* convert : to / */
      }
      fn = attr->fname; 	   /* take whole name */
      /* Ensure where is terminated with a slash */
      if (jcr->where[wherelen-1] != '/' && fn[0] != '/') {
         pm_strcat(attr->ofname, "/");
      }   
      pm_strcat(attr->ofname, fn); /* copy rest of name */
      /*
       * Fixup link name -- if it is an absolute path
       */
      if (attr->type == FT_LNKSAVED || attr->type == FT_LNK) {
	 bool add_link;
	 /* Always add prefix to hard links (FT_LNKSAVED) and
	  *  on user request to soft links
	  */
         if (attr->lname[0] == '/' &&
	     (attr->type == FT_LNKSAVED || jcr->prefix_links)) {
	    pm_strcpy(attr->olname, jcr->where);
	    add_link = true;
	 } else {
	    attr->olname[0] = 0;
	    add_link = false;
	 }
         if (win32_client && attr->lname[1] == ':') {
            attr->lname[1] = '/';    /* turn : into / */
	 }
	 fn = attr->lname;	 /* take whole name */
	 /* Ensure where is terminated with a slash */
         if (add_link && jcr->where[wherelen-1] != '/' && fn[0] != '/') {
            pm_strcat(attr->olname, "/");
	 }   
	 pm_strcat(attr->olname, fn);	  /* copy rest of link */
      }
   }
}

extern char *getuser(uid_t uid, char *name, int len);
extern char *getgroup(gid_t gid, char *name, int len);

/*
 * Print an ls style message, also send M_RESTORED
 */
void print_ls_output(JCR *jcr, ATTR *attr)
{
   char buf[5000]; 
   char ec1[30];
   char en1[30], en2[30];
   char *p, *f;

   p = encode_mode(attr->statp.st_mode, buf);
   p += sprintf(p, "  %2d ", (uint32_t)attr->statp.st_nlink);
   p += sprintf(p, "%-8.8s %-8.8s", getuser(attr->statp.st_uid, en1, sizeof(en1)),
		getgroup(attr->statp.st_gid, en2, sizeof(en2)));
   p += sprintf(p, "%8.8s ", edit_uint64(attr->statp.st_size, ec1));
   p = encode_time(attr->statp.st_ctime, p);
   *p++ = ' ';
   *p++ = ' ';
   for (f=attr->ofname; *f && (p-buf) < (int)sizeof(buf)-10; ) {
      *p++ = *f++;
   }
   if (attr->type == FT_LNK) {
      *p++ = ' ';
      *p++ = '-';
      *p++ = '>';
      *p++ = ' ';
      /* Copy link name */
      for (f=attr->olname; *f && (p-buf) < (int)sizeof(buf)-10; ) {
	 *p++ = *f++;
      }
   }
   *p++ = '\n';
   *p = 0;
   Dmsg1(20, "%s", buf);
   Jmsg(jcr, M_RESTORED, 1, "%s", buf);
}
