/*
 *   Match Bootstrap Records (used for restores) against
 *     Volume Records
 *
 *     Kern Sibbald, June MMII
 *
 *   Version $Id: match_bsr.c,v 1.21.8.1 2005/02/14 10:02:27 kerns Exp $
 */

/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

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
#include "stored.h"
#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include "lib/fnmatch.h"
#endif

/* Forward references */
static int match_volume(BSR *bsr, BSR_VOLUME *volume, VOLUME_LABEL *volrec, bool done);
static int match_sesstime(BSR *bsr, BSR_SESSTIME *sesstime, DEV_RECORD *rec, bool done);
static int match_sessid(BSR *bsr, BSR_SESSID *sessid, DEV_RECORD *rec);
static int match_client(BSR *bsr, BSR_CLIENT *client, SESSION_LABEL *sessrec, bool done);
static int match_job(BSR *bsr, BSR_JOB *job, SESSION_LABEL *sessrec, bool done);
static int match_job_type(BSR *bsr, BSR_JOBTYPE *job_type, SESSION_LABEL *sessrec, bool done);
static int match_job_level(BSR *bsr, BSR_JOBLEVEL *job_level, SESSION_LABEL *sessrec, bool done);
static int match_jobid(BSR *bsr, BSR_JOBID *jobid, SESSION_LABEL *sessrec, bool done);
static int match_findex(BSR *bsr, BSR_FINDEX *findex, DEV_RECORD *rec, bool done);
static int match_volfile(BSR *bsr, BSR_VOLFILE *volfile, DEV_RECORD *rec, bool done);
static int match_stream(BSR *bsr, BSR_STREAM *stream, DEV_RECORD *rec, bool done);
static int match_all(BSR *bsr, DEV_RECORD *rec, VOLUME_LABEL *volrec, SESSION_LABEL *sessrec, bool done);
static int match_block_sesstime(BSR *bsr, BSR_SESSTIME *sesstime, DEV_BLOCK *block);
static int match_block_sessid(BSR *bsr, BSR_SESSID *sessid, DEV_BLOCK *block);
static BSR *find_smallest_volfile(BSR *fbsr, BSR *bsr);


/*********************************************************************
 *
 *  If possible, position the archive device (tape) to read the
 *  next block.
 */
void position_bsr_block(BSR *bsr, DEV_BLOCK *block)
{
   /* To be implemented */
}

/*********************************************************************
 *
 *  Do fast block rejection based on bootstrap records.
 *    use_fast_rejection will be set if we have VolSessionId and VolSessTime
 *    in each record. When BlockVer is >= 2, we have those in the block header
 *    so can do fast rejection.
 *
 *   returns:  1 if block may contain valid records
 *	       0 if block may be skipped (i.e. it contains no records of
 *		    that can match the bsr).
 *
 */
int match_bsr_block(BSR *bsr, DEV_BLOCK *block)
{
   if (!bsr || !bsr->use_fast_rejection || (block->BlockVer < 2)) {
      return 1; 		      /* cannot fast reject */
   }

   for ( ; bsr; bsr=bsr->next) {
      if (!match_block_sesstime(bsr, bsr->sesstime, block)) {
	 continue;
      }
      if (!match_block_sessid(bsr, bsr->sessid, block)) {
	 continue;
      }
      return 1;
   }
   return 0;
}

static int match_block_sesstime(BSR *bsr, BSR_SESSTIME *sesstime, DEV_BLOCK *block)
{
   if (!sesstime) {
      return 1; 		      /* no specification matches all */
   }
   if (sesstime->sesstime == block->VolSessionTime) {
      return 1;
   }
   if (sesstime->next) {
      return match_block_sesstime(bsr, sesstime->next, block);
   }
   return 0;
}

static int match_block_sessid(BSR *bsr, BSR_SESSID *sessid, DEV_BLOCK *block)
{
   if (!sessid) {
      return 1; 		      /* no specification matches all */
   }
   if (sessid->sessid <= block->VolSessionId && sessid->sessid2 >= block->VolSessionId) {
      return 1;
   }
   if (sessid->next) {
      return match_block_sessid(bsr, sessid->next, block);
   }
   return 0;
}


/*********************************************************************
 *
 *	Match Bootstrap records
 *	  returns  1 on match
 *	  returns  0 no match and reposition is set if we should
 *			reposition the tape
 *	 returns -1 no additional matches possible
 */
int match_bsr(BSR *bsr, DEV_RECORD *rec, VOLUME_LABEL *volrec, SESSION_LABEL *sessrec)
{
   int stat;

   /*
    * The bsr->reposition flag is set any time a bsr is done.
    *	In this case, we can probably reposition the
    *	tape to the next available bsr position.
    */
   if (bsr) {
      bsr->reposition = false;
      stat = match_all(bsr, rec, volrec, sessrec, true);
      /*
       * Note, bsr->reposition is set by match_all when
       *  a bsr is done. We turn it off if a match was
       *  found or if we cannot use positioning
       */
      if (stat != 0 || !bsr->use_positioning) {
	 bsr->reposition = false;
      }
   } else {
      stat = 1; 		      /* no bsr => match all */
   }
   return stat;
}

/*
 * Find the next bsr that applies to the current tape.
 *   It is the one with the smallest VolFile position.
 */
BSR *find_next_bsr(BSR *root_bsr, DEVICE *dev)
{
   BSR *bsr;
   BSR *found_bsr = NULL;

   if (!root_bsr || !root_bsr->use_positioning ||
       !root_bsr->reposition || !dev_is_tape(dev)) {
      Dmsg2(100, "No nxt_bsr use_pos=%d repos=%d\n", root_bsr->use_positioning, root_bsr->reposition);
      return NULL;
   }
   Dmsg2(100, "use_pos=%d repos=%d\n", root_bsr->use_positioning, root_bsr->reposition);
   root_bsr->mount_next_volume = false;
   for (bsr=root_bsr; bsr; bsr=bsr->next) {
      if (bsr->done || !match_volume(bsr, bsr->volume, &dev->VolHdr, 1)) {
	 continue;
      }
      if (found_bsr == NULL) {
	 found_bsr = bsr;
      } else {
	 found_bsr = find_smallest_volfile(found_bsr, bsr);
      }
   }
   /*
    * If we get to this point and found no bsr, it means
    *  that any additional bsr's must apply to the next
    *  tape, so set a flag.
    */
   if (found_bsr == NULL) {
      root_bsr->mount_next_volume = true;
   }
   return found_bsr;
}

static BSR *find_smallest_volfile(BSR *found_bsr, BSR *bsr)
{
   BSR *return_bsr = found_bsr;
   BSR_VOLFILE *vf;
   BSR_VOLBLOCK *vb;
   uint32_t found_bsr_sfile, bsr_sfile;
   uint32_t found_bsr_sblock, bsr_sblock;

   vf = found_bsr->volfile;
   found_bsr_sfile = vf->sfile;
   while ( (vf=vf->next) ) {
      if (vf->sfile < found_bsr_sfile) {
	 found_bsr_sfile = vf->sfile;
      }
   }
   vf = bsr->volfile;
   bsr_sfile = vf->sfile;
   while ( (vf=vf->next) ) {
      if (vf->sfile < bsr_sfile) {
	 bsr_sfile = vf->sfile;
      }
   }
   if (found_bsr_sfile > bsr_sfile) {
      return_bsr = bsr;
   } else if (found_bsr_sfile == bsr_sfile) {
      /* Must check block */
      vb = found_bsr->volblock;
      found_bsr_sblock = vb->sblock;
      while ( (vb=vb->next) ) {
	 if (vb->sblock < found_bsr_sblock) {
	    found_bsr_sblock = vb->sblock;
	 }
      }
      vb = bsr->volblock;
      bsr_sblock = vb->sblock;
      while ( (vb=vb->next) ) {
	 if (vb->sblock < bsr_sblock) {
	    bsr_sblock = vb->sblock;
	 }
      }
      if (found_bsr_sblock > bsr_sblock) {
	 return_bsr = bsr;
      }
   }

   return return_bsr;
}

/*
 * Called to tell the matcher that the end of
 *   the current file has been reached.
 *  The bsr argument is not used, but is included
 *    for consistency with the other match calls.
 *
 * Returns: true if we should reposition
 *	  : false otherwise.
 */
bool match_set_eof(BSR *bsr, DEV_RECORD *rec)
{
   BSR *rbsr = rec->bsr;
   Dmsg1(100, "match_set %d\n", rbsr != NULL);
   if (!rbsr) {
      return false;
   }
   rec->bsr = NULL;
   rbsr->found++;
   if (rbsr->count && rbsr->found >= rbsr->count) {
      rbsr->done = true;
      rbsr->root->reposition = true;
      Dmsg2(100, "match_set_eof reposition count=%d found=%d\n",
	 rbsr->count, rbsr->found);
      return true;
   }
   return false;
}

/*
 * Match all the components of current record
 *   returns  1 on match
 *   returns  0 no match
 *   returns -1 no additional matches possible
 */
static int match_all(BSR *bsr, DEV_RECORD *rec, VOLUME_LABEL *volrec,
		     SESSION_LABEL *sessrec, bool done)
{
   if (bsr->done) {
      goto no_match;
   }
   if (!match_volume(bsr, bsr->volume, volrec, 1)) {
      goto no_match;
   }
   if (!match_volfile(bsr, bsr->volfile, rec, 1)) {
      Dmsg2(100, "Fail on file. bsr=%d rec=%d\n", bsr->volfile->efile,
	 rec->File);
      goto no_match;
   }
   if (!match_sesstime(bsr, bsr->sesstime, rec, 1)) {
      Dmsg2(100, "Fail on sesstime. bsr=%d rec=%d\n",
	 bsr->sesstime->sesstime, rec->VolSessionTime);
      goto no_match;
   }

   /* NOTE!! This test MUST come after the sesstime test */
   if (!match_sessid(bsr, bsr->sessid, rec)) {
      Dmsg2(100, "Fail on sessid. bsr=%d rec=%d\n",
	 bsr->sessid->sessid, rec->VolSessionId);
      goto no_match;
   }

   /* NOTE!! This test MUST come after sesstime and sessid tests */
   if (!match_findex(bsr, bsr->FileIndex, rec, 1)) {
      Dmsg2(100, "Fail on findex. bsr=%d rec=%d\n",
	 bsr->FileIndex->findex2, rec->FileIndex);
      goto no_match;
   }
   /*
    * If a count was specified and we have a FileIndex, assume
    *	it is a Bacula created bsr (or the equivalent). We
    *	then save the bsr where the match occurred so that
    *	after processing the record or records, we can update
    *	the found count. I.e. rec->bsr points to the bsr that
    *	satisfied the match.
    */
   if (bsr->count && bsr->FileIndex) {
      rec->bsr = bsr;
      return 1; 		      /* this is a complete match */
   }

   /*
    * The selections below are not used by Bacula's
    *   restore command, and don't work because of
    *	the rec->bsr = bsr optimization above.
    */
   if (!match_jobid(bsr, bsr->JobId, sessrec, 1)) {
      goto no_match;

   }
   if (!match_job(bsr, bsr->job, sessrec, 1)) {
      goto no_match;
   }
   if (!match_client(bsr, bsr->client, sessrec, 1)) {
      goto no_match;
   }
   if (!match_job_type(bsr, bsr->JobType, sessrec, 1)) {
      goto no_match;
   }
   if (!match_job_level(bsr, bsr->JobLevel, sessrec, 1)) {
      goto no_match;
   }
   if (!match_stream(bsr, bsr->stream, rec, 1)) {
      goto no_match;
   }
   return 1;

no_match:
   if (bsr->next) {
      return match_all(bsr->next, rec, volrec, sessrec, bsr->done && done);
   }
   if (bsr->done && done) {
      return -1;
   }
   return 0;
}

static int match_volume(BSR *bsr, BSR_VOLUME *volume, VOLUME_LABEL *volrec, bool done)
{
   if (!volume) {
      return 0; 		      /* Volume must match */
   }
   if (strcmp(volume->VolumeName, volrec->VolName) == 0) {
      return 1;
   }
   if (volume->next) {
      return match_volume(bsr, volume->next, volrec, 1);
   }
   return 0;
}

static int match_client(BSR *bsr, BSR_CLIENT *client, SESSION_LABEL *sessrec, bool done)
{
   if (!client) {
      return 1; 		      /* no specification matches all */
   }
   if (fnmatch(client->ClientName, sessrec->ClientName, 0) == 0) {
      return 1;
   }
   if (client->next) {
      return match_client(bsr, client->next, sessrec, 1);
   }
   return 0;
}

static int match_job(BSR *bsr, BSR_JOB *job, SESSION_LABEL *sessrec, bool done)
{
   if (!job) {
      return 1; 		      /* no specification matches all */
   }
   if (fnmatch(job->Job, sessrec->Job, 0) == 0) {
      return 1;
   }
   if (job->next) {
      return match_job(bsr, job->next, sessrec, 1);
   }
   return 0;
}

static int match_job_type(BSR *bsr, BSR_JOBTYPE *job_type, SESSION_LABEL *sessrec, bool done)
{
   if (!job_type) {
      return 1; 		      /* no specification matches all */
   }
   if (job_type->JobType == sessrec->JobType) {
      return 1;
   }
   if (job_type->next) {
      return match_job_type(bsr, job_type->next, sessrec, 1);
   }
   return 0;
}

static int match_job_level(BSR *bsr, BSR_JOBLEVEL *job_level, SESSION_LABEL *sessrec, bool done)
{
   if (!job_level) {
      return 1; 		      /* no specification matches all */
   }
   if (job_level->JobLevel == sessrec->JobLevel) {
      return 1;
   }
   if (job_level->next) {
      return match_job_level(bsr, job_level->next, sessrec, 1);
   }
   return 0;
}

static int match_jobid(BSR *bsr, BSR_JOBID *jobid, SESSION_LABEL *sessrec, bool done)
{
   if (!jobid) {
      return 1; 		      /* no specification matches all */
   }
   if (jobid->JobId <= sessrec->JobId && jobid->JobId2 >= sessrec->JobId) {
      return 1;
   }
   if (jobid->next) {
      return match_jobid(bsr, jobid->next, sessrec, 1);
   }
   return 0;
}

static int match_volfile(BSR *bsr, BSR_VOLFILE *volfile, DEV_RECORD *rec, bool done)
{
   if (!volfile) {
      return 1; 		      /* no specification matches all */
   }
   /* For the moment, these tests work only with tapes. */
   if (!(rec->state & REC_ISTAPE)) {
      return 1; 		      /* All File records OK for this match */
   }
// Dmsg3(100, "match_volfile: sfile=%d efile=%d recfile=%d\n",
//	       volfile->sfile, volfile->efile, rec->File);
   if (volfile->sfile <= rec->File && volfile->efile >= rec->File) {
      return 1;
   }
   /* Once we get past last efile, we are done */
   if (rec->File > volfile->efile) {
      volfile->done = true;		 /* set local done */
   }
   if (volfile->next) {
      return match_volfile(bsr, volfile->next, rec, volfile->done && done);
   }

   /* If we are done and all prior matches are done, this bsr is finished */
   if (volfile->done && done) {
      bsr->done = true;
      bsr->root->reposition = true;
      Dmsg2(100, "bsr done from volfile rec=%d volefile=%d\n",
	 rec->File, volfile->efile);
   }
   return 0;
}

static int match_stream(BSR *bsr, BSR_STREAM *stream, DEV_RECORD *rec, bool done)
{
   if (!stream) {
      return 1; 		      /* no specification matches all */
   }
   if (stream->stream == rec->Stream) {
      return 1;
   }
   if (stream->next) {
      return match_stream(bsr, stream->next, rec, 1);
   }
   return 0;
}

static int match_sesstime(BSR *bsr, BSR_SESSTIME *sesstime, DEV_RECORD *rec, bool done)
{
   if (!sesstime) {
      return 1; 		      /* no specification matches all */
   }
   if (sesstime->sesstime == rec->VolSessionTime) {
      return 1;
   }
   if (rec->VolSessionTime > sesstime->sesstime) {
      sesstime->done = true;
   }
   if (sesstime->next) {
      return match_sesstime(bsr, sesstime->next, rec, sesstime->done && done);
   }
   if (sesstime->done && done) {
      bsr->done = true;
      bsr->root->reposition = true;
      Dmsg0(100, "bsr done from sesstime\n");
   }
   return 0;
}

static int match_sessid(BSR *bsr, BSR_SESSID *sessid, DEV_RECORD *rec)
{
   if (!sessid) {
      return 1; 		      /* no specification matches all */
   }
   if (sessid->sessid <= rec->VolSessionId && sessid->sessid2 >= rec->VolSessionId) {
      return 1;
   }
   if (sessid->next) {
      return match_sessid(bsr, sessid->next, rec);
   }
   return 0;
}

/*
 * When reading the Volume, the Volume Findex (rec->FileIndex) always
 *   are found in sequential order. Thus we can make optimizations.
 */
static int match_findex(BSR *bsr, BSR_FINDEX *findex, DEV_RECORD *rec, bool done)
{
   if (!findex) {
      return 1; 		      /* no specification matches all */
   }
   if (findex->findex <= rec->FileIndex && findex->findex2 >= rec->FileIndex) {
      return 1;
   }
   if (rec->FileIndex > findex->findex2) {
      findex->done = true;
   }
   if (findex->next) {
      return match_findex(bsr, findex->next, rec, findex->done && done);
   }
   if (findex->done && done) {
      bsr->done = true;
      bsr->root->reposition = true;
      Dmsg1(100, "bsr done from findex %d\n", rec->FileIndex);
   }
   return 0;
}
