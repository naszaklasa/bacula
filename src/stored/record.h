/*
 * Record, and label definitions for Bacula
 *  media data format.
 *
 *   Kern Sibbald, MM
 *
 *   Version $Id: record.h,v 1.17 2004/09/19 18:56:29 kerns Exp $
 *
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


#ifndef __RECORD_H
#define __RECORD_H 1

/* Return codes from read_device_volume_label() */
#define VOL_NOT_READ      0               /* Volume label not read */
#define VOL_OK            1               /* volume name OK */
#define VOL_NO_LABEL      2               /* volume not labeled */
#define VOL_IO_ERROR      3               /* volume I/O error */
#define VOL_NAME_ERROR    4               /* Volume name mismatch */
#define VOL_CREATE_ERROR  5               /* Error creating label */
#define VOL_VERSION_ERROR 6               /* Bacula version error */
#define VOL_LABEL_ERROR   7               /* Bad label type */
#define VOL_NO_MEDIA      8               /* Hard error -- no media present */


/*  See block.h for RECHDR_LENGTH */

/*
 * This is the Media structure for a record header.
 *  NB: when it is written it is serialized.

   uint32_t VolSessionId;
   uint32_t VolSessionTime;

 * The above 8 bytes are only written in a BB01 block, BB02
 *  and later blocks contain these values in the block header
 *  rather than the record header.

   int32_t  FileIndex;
   int32_t  Stream;
   uint32_t data_len;

 */

/* Record state bit definitions */
#define REC_NO_HEADER        (1<<0)   /* No header read */
#define REC_PARTIAL_RECORD   (1<<1)   /* returning partial record */
#define REC_BLOCK_EMPTY      (1<<2)   /* not enough data in block */
#define REC_NO_MATCH         (1<<3)   /* No match on continuation data */
#define REC_CONTINUATION     (1<<4)   /* Continuation record found */
#define REC_ISTAPE           (1<<5)   /* Set if device is tape */

#define is_partial_record(r) ((r)->state & REC_PARTIAL_RECORD)
#define is_block_empty(r)    ((r)->state & REC_BLOCK_EMPTY)

/*
 * DEV_RECORD for reading and writing records.
 * It consists of a Record Header, and the Record Data
 * 
 *  This is the memory structure for the record header.
 */
struct BSR;                           /* satisfy forward reference */
struct DEV_RECORD {
   dlink link;                        /* link for chaining in read_record.c */
   /* File and Block are always returned during reading
    *  and writing records.
    */
   uint32_t File;                     /* File number */
   uint32_t Block;                    /* Block number */
   uint32_t VolSessionId;             /* sequential id within this session */
   uint32_t VolSessionTime;           /* session start time */
   int32_t  FileIndex;                /* sequential file number */
   int32_t  Stream;                   /* stream number */
   uint32_t data_len;                 /* current record length */
   uint32_t remainder;                /* remaining bytes to read/write */
   uint32_t state;                    /* state bits */
   BSR *bsr;                          /* pointer to bsr that matched */
   uint8_t  ser_buf[WRITE_RECHDR_LENGTH];   /* serialized record header goes here */
   POOLMEM *data;                     /* Record data. This MUST be a memory pool item */
};           


/*
 * Values for LabelType that are put into the FileIndex field
 * Note, these values are negative to distinguish them
 * from user records where the FileIndex is forced positive.
 */
#define PRE_LABEL   -1                /* Vol label on unwritten tape */
#define VOL_LABEL   -2                /* Volume label first file */
#define EOM_LABEL   -3                /* Writen at end of tape */        
#define SOS_LABEL   -4                /* Start of Session */
#define EOS_LABEL   -5                /* End of Session */
#define EOT_LABEL   -6                /* End of physical tape (2 eofs) */

/* 
 *   Volume Label Record.  This is the in-memory definition. The
 *     tape definition is defined in the serialization code itself
 *     ser_volume_label() and unser_volume_label() and is slightly different.
 */

 
struct Volume_Label {
  /*  
   * The first items in this structure are saved
   * in the DEVICE buffer, but are not actually written
   * to the tape.
   */
  int32_t LabelType;                  /* This is written in header only */
  uint32_t LabelSize;                 /* length of serialized label */
  /*
   * The items below this line are stored on 
   * the tape
   */
  char Id[32];                        /* Bacula Immortal ... */

  uint32_t VerNum;                    /* Label version number */

  /* VerNum <= 10 */
  float64_t label_date;               /* Date tape labeled */
  float64_t label_time;               /* Time tape labeled */

  /* VerNum >= 11 */
  btime_t   label_btime;              /* tdate tape labeled */
  btime_t   write_btime;              /* tdate tape written */

  /* Unused with VerNum >= 11 */
  float64_t write_date;               /* Date this label written */
  float64_t write_time;               /* Time this label written */

  char VolName[MAX_NAME_LENGTH];      /* Volume name */
  char PrevVolName[MAX_NAME_LENGTH];  /* Previous Volume Name */
  char PoolName[MAX_NAME_LENGTH];     /* Pool name */
  char PoolType[MAX_NAME_LENGTH];     /* Pool type */
  char MediaType[MAX_NAME_LENGTH];    /* Type of this media */

  char HostName[MAX_NAME_LENGTH];     /* Host name of writing computer */
  char LabelProg[50];                 /* Label program name */
  char ProgVersion[50];               /* Program version */
  char ProgDate[50];                  /* Program build date/time */
};

#define SER_LENGTH_Volume_Label 1024   /* max serialised length of volume label */
#define SER_LENGTH_Session_Label 1024  /* max serialised length of session label */

typedef struct Volume_Label VOLUME_LABEL;

/*
 * Session Start/End Label
 *  This record is at the beginning and end of each session
 */
struct Session_Label {
  char Id[32];                        /* Bacula Immortal ... */

  uint32_t VerNum;                    /* Label version number */

  uint32_t JobId;                     /* Job id */
  uint32_t VolumeIndex;               /* Sequence no of volume for this job */

  /* VerNum >= 11 */
  btime_t   write_btime;              /* Tdate this label written */

  /* VerNum < 11 */
  float64_t write_date;               /* Date this label written */

  /* Unused VerNum >= 11 */
  float64_t write_time;               /* Time this label written */

  char PoolName[MAX_NAME_LENGTH];     /* Pool name */
  char PoolType[MAX_NAME_LENGTH];     /* Pool type */
  char JobName[MAX_NAME_LENGTH];      /* base Job name */
  char ClientName[MAX_NAME_LENGTH];
  char Job[MAX_NAME_LENGTH];          /* Unique name of this Job */
  char FileSetName[MAX_NAME_LENGTH];
  char FileSetMD5[MAX_NAME_LENGTH];
  uint32_t JobType;
  uint32_t JobLevel;
  /* The remainder are part of EOS label only */
  uint32_t JobFiles;
  uint64_t JobBytes;
  uint32_t StartBlock;
  uint32_t EndBlock;
  uint32_t StartFile;
  uint32_t EndFile;
  uint32_t JobErrors;
  uint32_t JobStatus;                 /* Job status */

};
typedef struct Session_Label SESSION_LABEL;

#define SERIAL_BUFSIZE  1024          /* volume serialisation buffer size */

#endif
