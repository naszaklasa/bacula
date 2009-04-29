/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2006-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 * mtops.c - Emulate the Linux st (scsi tape) driver on file.
 * for regression and bug hunting purpose
 *
 * Version $Id$
 *
 */

#include <stdarg.h>
#include <stddef.h>

#include "bacula.h"                   /* pull in global headers */
#include "stored.h"                   /* pull in Storage Deamon headers */

#include "sys/mtio.h"

//
// SCSI bus status codes.
//

#define SCSISTAT_GOOD                  0x00
#define SCSISTAT_CHECK_CONDITION       0x02
#define SCSISTAT_CONDITION_MET         0x04
#define SCSISTAT_BUSY                  0x08
#define SCSISTAT_INTERMEDIATE          0x10
#define SCSISTAT_INTERMEDIATE_COND_MET 0x14
#define SCSISTAT_RESERVATION_CONFLICT  0x18
#define SCSISTAT_COMMAND_TERMINATED    0x22
#define SCSISTAT_QUEUE_FULL            0x28

/*
 * +---+-------+----------------+------------------------+
 * | V | NB FM | FM index tab   | Data                   |
 * +---+-------+----------------+------------------------+
 *
 * V : char version
 * NB FM : int16 max_file_mark
 * FM index : int64 file_mark_index[max_file_mark]
 * DATA : data
 */

typedef struct
{
   /* format infos */
   int16_t     version;
   int16_t     max_file_mark;
   int16_t     block_size;
   int32_t     block_max;
   off_t       *file_mark_index;
   off_t       data_start;

} FTAPE_FORMAT;

#define FTAPE_HEADER_SIZE (2+sizeof(int16_t))

typedef  struct
{
   int         fd : 0;		/* Os file descriptor */
   char        *file;		/* file name */
   bool        EOD;
   bool        EOF;		/* End of file */
   bool        EOT;		/* End of tape */

   FTAPE_FORMAT format;

   int16_t     current_file;	/* max 65000 files */
   int32_t     current_block;	/* max 4G blocks of 512B */

} FTAPE_INFO;

#define NB_TAPE_LIST  15
FTAPE_INFO tape_list[NB_TAPE_LIST];


static int tape_get(int fd, struct mtget *mt_get);
static int tape_op(int fd, struct mtop *mt_com);
static int tape_pos(int fd, struct mtpos *mt_pos);

static int dbglevel = 10;

static FTAPE_INFO *get_tape(int fd)
{
   if (fd >= NB_TAPE_LIST) {
      /* error */
      return NULL;
   }

   return &tape_list[fd];
}

static FTAPE_INFO *new_volume(int fd, const char *file)
{
   FTAPE_INFO *tape = get_tape(fd);
   if (!tape) {
      return NULL;
   }
   memset(tape, 0, sizeof(FTAPE_INFO));

   tape->fd = fd;
   tape->file = bstrdup(file);

   nb = read(tape->fd, &tape->format, FTAPE_HEADER_SIZE);
   if (nb != FTAPE_HEADER_SIZE) { /* new tape ? */
      
   }
   tape->format.file_index = mmap(NULL, 
				  tape->format.max_file_mark * sizeof(off_t), 
				  PROT_READ | PROT_WRITE,
				  MAP_SHARED, fileno(fd), FTAPE_HEADER_SIZE);

   if (!tape->format.file_index) {
      /* error */
   }

   tape->format.data_start = 
      FTAPE_HEADER_SIZE  + sizeof(off_t)* tape->format.max_file_mark ;

   return tape;
}

static int free_volume(int fd)
{
   FTAPE_INFO *tape = get_tape(fd);

   if (tape) {
      munmap(tape->format.file_index, tape->format.max_file_mark * sizeof(off_t));
      free(tape->file);
      free(tape);
   }
}

int
fake_tape_open(const char *file, int flags, int mode)
{
   int fd;
   Dmsg(dbglevel, "fake_tape_open(%s, %i, %i)\n", file, flags, mode);
   fd = open(file, flags, mode);
   new_volume(fd, file);
   return fd;
}

int
fake_tape_read(int fd, void *buffer, unsigned int count)
{
   int ret;
   if (buffer == NULL) {
      errno = EINVAL;
      return -1;
   }
   ret = read(fd, buffer, count);
   return ret;
}

int
fake_tape_write(int fd, const void *buffer, unsigned int count)
{
   int ret;
   if (buffer == NULL) {
      errno = EINVAL;
      return -1;
   }
   ret = write(fd, buffer, count);
   return ret;
}

int
fake_tape_close(int fd)
{
   free_volume(fd);
   close(fd);
   return 0;
}

int
fake_tape_ioctl(int fd, unsigned long int request, ...)
{
   va_list argp;
   int result;

   va_start(argp, request);

   switch (request) {
   case MTIOCTOP:
      result = tape_op(fd, va_arg(argp, mtop *));
      break;

   case MTIOCGET:
      result = tape_get(fd, va_arg(argp, mtget *));
      break;

   case MTIOCPOS:
      result = tape_pos(fd, va_arg(argp, mtpos *));
      break;

   default:
      errno = ENOTTY;
      result = -1;
      break;
   }

   va_end(argp);

   return result;
}

static int tape_op(int fd, struct mtop *mt_com)
{
   int result;
   
   FTAPE_INFO *tape = get_tape(fd);
   if (!tape) {
      errno = EBADF;
      return -1;
   }
   
   switch (mt_com->mt_op)
   {
   case MTRESET:
   case MTNOP:
   case MTSETDRVBUFFER:
      break;

   default:
   case MTRAS1:
   case MTRAS2:
   case MTRAS3:
   case MTSETDENSITY:
      errno = ENOTTY;
      result = (DWORD)-1;
      break;

   case MTFSF:
      index = tape->current_file + mt_com->mt_count;

      if (index >= tape->max_file_mark) {
	 errno = EIO;
	 result = -1;

      } else {
	 tape->current_file = index;
	 tape->current_block = 0;
	 tape->EOF = true;
	 tape->EOT = false;

	 pos = tape->file_mark_index[index];
	 if (lseek(fd, pos, SEEK_SET) == (boffset_t)-1) {
	    berrno be;
	    dev_errno = errno;
	    Mmsg2(errmsg, _("lseek error on %s. ERR=%s.\n"),
		  print_name(), be.bstrerror());
	    result = -1;
	 }
      }
      break;

   case MTBSF:
      index = tape->current_file - mt_com->mt_count;

      if (index < 0 && index => tape->max_file_mark) {
	 errno = EIO;
	 result = -1;

      } else {
	 tape->current_file = index;
	 tape->current_block = 0;
	 tape->EOF = true;
	 tape->EOT = false;

	 pos = tape->file_mark_index[index];
	 if (lseek(fd, pos, SEEK_SET) == (boffset_t)-1) {
	    berrno be;
	    dev_errno = errno;
	    Mmsg2(errmsg, _("lseek error on %s. ERR=%s.\n"),
		  print_name(), be.bstrerror());
	    result = -1;
	 }
      break;

   case MTFSR:			/* block forward */
      if ((tape->current_block + mt_com->mt_count) >= tape->block_max) {
	 errno = ENOSPC;
	 result = -1;

      } else { // TODO: check for tape mark

	 tape->current_block += mt_com->mt_count;
	 tape->EOF = false;
	 tape->EOT = false;

	 if (lseek(fd, mt_com->mt_count * tape->format.block_size, SEEK_CUR) == (boffset_t)-1) {
	    berrno be;
	    dev_errno = errno;
	    Mmsg2(errmsg, _("lseek error on %s. ERR=%s.\n"),
		  print_name(), be.bstrerror());
	    result = -1;
	 }
      }

//
//      result = SetTapePosition(pHandleInfo->OSHandle, TAPE_SPACE_RELATIVE_BLOCKS, 0, mt_com->mt_count, 0, FALSE);
//      if (result == NO_ERROR) {
//         pHandleInfo->bEOD = false;
//         pHandleInfo->bEOF = false;
//         pHandleInfo->bEOT = false;
//      } else if (result == ERROR_FILEMARK_DETECTED) {
//         pHandleInfo->bEOF = true;
//      }
//
      break;

   case MTBSR:
      if ((tape->current_block - mt_com->mt_count) < 0) {
         // we are on tapemark, we have to BSF
	 struct mtop mt;
	 mt.mt_op = MTBSF;
	 mt.mt_count = 1;
	 result = tape_op(fd, &mt);
	 
      } else { 

	 tape->current_block -= mt_com->mt_count;
	 tape->EOF = false;
	 tape->EOT = false;
	 
	 if (lseek(fd, -mt_com->mt_count * tape->format.block_size, SEEK_CUR) == (boffset_t)-1) {
	    berrno be;
	    dev_errno = errno;
	    Mmsg2(errmsg, _("lseek error on %s. ERR=%s.\n"),
		  print_name(), be.bstrerror());
	    result = -1;
	 }
      }
      break;

   case MTWEOF:
      if (tape->current_file >= tape->format.max_file_mark) {
	 errno = ENOSPC;
	 result = -1;
      } else {
	 tape->current_file++;
	 tape->format.file_index[tape->current_file] = ftell(fd);
	 tape->EOF = 1;
      }

      break;

   case MTREW:
      if (lseek(fd, tape->format.data_start, SEEK_SET) < 0) {
         berrno be;
         dev_errno = errno;
         Mmsg2(errmsg, _("lseek error on %s. ERR=%s.\n"),
            print_name(), be.bstrerror());
         result = -1 ;
      } else {
	 tape->EOF = false;
	 result = NO_ERROR;
      }
      break;

   case MTOFFL:			/* put tape offline */
      // check if can_read/can_write
      result = NO_ERROR;
      break;

   case MTRETEN:
      result = NO_ERROR;
      break;

   case MTBSFM:			/* not used by bacula */
      errno = EIO;
      result = -1;
      break;

   case MTFSFM:			/* not used by bacula */
      errno = EIO;
      result = -1;
      break;

   case MTEOM:
      int i=0;
      int last=0;
      for (i=tape->format.current_file;
	   (i < tape->format.max_file_mark) &&(tape->format.file_index[i] != 0) ; 
	   i++)
      {
	 last = i;
      }

      struct mtop mt;
      mt.mt_op = MTFSF;
      mt.mt_count = last;
      result = tape_op(fd, &mt);

      break;

   case MTERASE:
      result = EraseTape(pHandleInfo->OSHandle, TAPE_ERASE_LONG, FALSE);
      if (result == NO_ERROR) {
         pHandleInfo->bEOD = true;
         pHandleInfo->bEOF = false;
         pHandleInfo->bEOT = false;
         pHandleInfo->ulFile = 0;
         pHandleInfo->bBlockValid = true;
         pHandleInfo->ullFileStart = 0;
      }
      break;

   case MTSETBLK:
      {
         TAPE_SET_MEDIA_PARAMETERS  SetMediaParameters;

         SetMediaParameters.BlockSize = mt_com->mt_count;
         result = SetTapeParameters(pHandleInfo->OSHandle, SET_TAPE_MEDIA_INFORMATION, &SetMediaParameters);
      }
      break;

   case MTSEEK:
      {
         TAPE_POSITION_INFO   TapePositionInfo;

         result = SetTapePosition(pHandleInfo->OSHandle, TAPE_ABSOLUTE_BLOCK, 0, mt_com->mt_count, 0, FALSE);

         memset(&TapePositionInfo, 0, sizeof(TapePositionInfo));
         DWORD dwPosResult = GetTapePositionInfo(pHandleInfo->OSHandle, &TapePositionInfo);
         if (dwPosResult == NO_ERROR && TapePositionInfo.FileSetValid) {
            pHandleInfo->ulFile = (ULONG)TapePositionInfo.FileNumber;
         } else {
            pHandleInfo->ulFile = ~0U;
         }
      }
      break;

   case MTTELL:
      {
         DWORD partition;
         DWORD offset;
         DWORD offsetHi;

         result = GetTapePosition(pHandleInfo->OSHandle, TAPE_ABSOLUTE_BLOCK, &partition, &offset, &offsetHi);
         if (result == NO_ERROR) {
            return offset;
         }
      }
      break;

   case MTFSS:
      result = SetTapePosition(pHandleInfo->OSHandle, TAPE_SPACE_SETMARKS, 0, mt_com->mt_count, 0, FALSE);
      break;

   case MTBSS:
      result = SetTapePosition(pHandleInfo->OSHandle, TAPE_SPACE_SETMARKS, 0, -mt_com->mt_count, ~0, FALSE);
      break;

   case MTWSM:
      result = WriteTapemark(pHandleInfo->OSHandle, TAPE_SETMARKS, mt_com->mt_count, FALSE);
      break;

   case MTLOCK:
      result = PrepareTape(pHandleInfo->OSHandle, TAPE_LOCK, FALSE);
      break;

   case MTUNLOCK:
      result = PrepareTape(pHandleInfo->OSHandle, TAPE_UNLOCK, FALSE);
      break;

   case MTLOAD:
      result = PrepareTape(pHandleInfo->OSHandle, TAPE_LOAD, FALSE);
      break;

   case MTUNLOAD:
      result = PrepareTape(pHandleInfo->OSHandle, TAPE_UNLOAD, FALSE);
      break;

   case MTCOMPRESSION:
      {
         TAPE_GET_DRIVE_PARAMETERS  GetDriveParameters;
         TAPE_SET_DRIVE_PARAMETERS  SetDriveParameters;
         DWORD                      size;

         size = sizeof(GetDriveParameters);

         result = GetTapeParameters(pHandleInfo->OSHandle, GET_TAPE_DRIVE_INFORMATION, &size, &GetDriveParameters);

         if (result == NO_ERROR)
         {
            SetDriveParameters.ECC = GetDriveParameters.ECC;
            SetDriveParameters.Compression = (BOOLEAN)mt_com->mt_count;
            SetDriveParameters.DataPadding = GetDriveParameters.DataPadding;
            SetDriveParameters.ReportSetmarks = GetDriveParameters.ReportSetmarks;
            SetDriveParameters.EOTWarningZoneSize = GetDriveParameters.EOTWarningZoneSize;

            result = SetTapeParameters(pHandleInfo->OSHandle, SET_TAPE_DRIVE_INFORMATION, &SetDriveParameters);
         }
      }
      break;

   case MTSETPART:
      result = SetTapePosition(pHandleInfo->OSHandle, TAPE_LOGICAL_BLOCK, mt_com->mt_count, 0, 0, FALSE);
      break;

   case MTMKPART:
      if (mt_com->mt_count == 0)
      {
         result = CreateTapePartition(pHandleInfo->OSHandle, TAPE_INITIATOR_PARTITIONS, 1, 0);
      }
      else
      {
         result = CreateTapePartition(pHandleInfo->OSHandle, TAPE_INITIATOR_PARTITIONS, 2, mt_com->mt_count);
      }
      break;
   }

   if ((result == NO_ERROR && pHandleInfo->bEOF) || 
       (result == ERROR_FILEMARK_DETECTED && mt_com->mt_op == MTFSR)) {

      TAPE_POSITION_INFO TapePositionInfo;

      if (GetTapePositionInfo(pHandleInfo->OSHandle, &TapePositionInfo) == NO_ERROR) {
         pHandleInfo->bBlockValid = true;
         pHandleInfo->ullFileStart = TapePositionInfo.BlockNumber;
      }
   }

   switch (result) {
   case NO_ERROR:
   case (DWORD)-1:   /* Error has already been translated into errno */
      break;

   default:
   case ERROR_FILEMARK_DETECTED:
      errno = EIO;
      break;

   case ERROR_END_OF_MEDIA:
      pHandleInfo->bEOT = true;
      errno = EIO;
      break;

   case ERROR_NO_DATA_DETECTED:
      pHandleInfo->bEOD = true;
      errno = EIO;
      break;

   case ERROR_NO_MEDIA_IN_DRIVE:
      pHandleInfo->bEOF = false;
      pHandleInfo->bEOT = false;
      pHandleInfo->bEOD = false;
      errno = ENOMEDIUM;
      break;

   case ERROR_INVALID_HANDLE:
   case ERROR_ACCESS_DENIED:
   case ERROR_LOCK_VIOLATION:
      errno = EBADF;
      break;
   }

   return result == NO_ERROR ? 0 : -1;
}

static int tape_get(int fd, struct mtget *mt_get)
{
   TAPE_POSITION_INFO pos_info;
   BOOL result;

   if (fd < 3 || fd >= (int)(NUMBER_HANDLE_ENTRIES + 3) || 
       TapeHandleTable[fd - 3].OSHandle == INVALID_HANDLE_VALUE) {
      errno = EBADF;
      return -1;
   }

   PTAPE_HANDLE_INFO    pHandleInfo = &TapeHandleTable[fd - 3];

   if (GetTapePositionInfo(pHandleInfo->OSHandle, &pos_info) != NO_ERROR) {
      return -1;
   }

   DWORD density = 0;
   DWORD blocksize = 0;

   result = GetDensityBlockSize(pHandleInfo->OSHandle, &density, &blocksize);

   if (result != NO_ERROR) {
      TAPE_GET_DRIVE_PARAMETERS drive_params;
      DWORD size;

      size = sizeof(drive_params);

      result = GetTapeParameters(pHandleInfo->OSHandle, GET_TAPE_DRIVE_INFORMATION, &size, &drive_params);

      if (result == NO_ERROR) {
         blocksize = drive_params.DefaultBlockSize;
      }
   }

   mt_get->mt_type = MT_ISSCSI2;

   // Partition #
   mt_get->mt_resid = pos_info.PartitionBlockValid ? pos_info.Partition : (ULONG)-1;

   // Density / Block Size
   mt_get->mt_dsreg = ((density << MT_ST_DENSITY_SHIFT) & MT_ST_DENSITY_MASK) |
                      ((blocksize << MT_ST_BLKSIZE_SHIFT) & MT_ST_BLKSIZE_MASK);

   mt_get->mt_gstat = 0x00010000;  /* Immediate report mode.*/

   if (pHandleInfo->bEOF) {
      mt_get->mt_gstat |= 0x80000000;     // GMT_EOF
   }

   if (pos_info.PartitionBlockValid && pos_info.BlockNumber == 0) {
      mt_get->mt_gstat |= 0x40000000;     // GMT_BOT
   }

   if (pHandleInfo->bEOT) {
      mt_get->mt_gstat |= 0x20000000;     // GMT_EOT
   }

   if (pHandleInfo->bEOD) {
      mt_get->mt_gstat |= 0x08000000;     // GMT_EOD
   }

   TAPE_GET_MEDIA_PARAMETERS  media_params;
   DWORD size = sizeof(media_params);
   
   result = GetTapeParameters(pHandleInfo->OSHandle, GET_TAPE_MEDIA_INFORMATION, &size, &media_params);

   if (result == NO_ERROR && media_params.WriteProtected) {
      mt_get->mt_gstat |= 0x04000000;     // GMT_WR_PROT
   }

   result = GetTapeStatus(pHandleInfo->OSHandle);

   if (result != NO_ERROR) {
      if (result == ERROR_NO_MEDIA_IN_DRIVE) {
         mt_get->mt_gstat |= 0x00040000;  // GMT_DR_OPEN
      }
   } else {
      mt_get->mt_gstat |= 0x01000000;     // GMT_ONLINE
   }

   // Recovered Error Count
   mt_get->mt_erreg = 0;

   // File Number
   mt_get->mt_fileno = (__daddr_t)pHandleInfo->ulFile;

   // Block Number
   mt_get->mt_blkno = (__daddr_t)(pHandleInfo->bBlockValid ? pos_info.BlockNumber - pHandleInfo->ullFileStart : (ULONGLONG)-1);

   return 0;
}
