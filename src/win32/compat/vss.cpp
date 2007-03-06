
// vss.cpp -- Interface to Volume Shadow Copies (VSS)
//
// Copyright transferred from MATRIX-Computer GmbH to
//   Kern Sibbald by express permission.
//
//  Copyright (C) 2005 Kern Sibbald
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  version 2 as amended with additional clauses defined in the
//  file LICENSE in the main source directory.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
//  the file LICENSE for additional details.
//
//  
// Author          : Thorsten Engel
// Created On      : Fri May 06 21:44:00 2005


#include <stdio.h>
#include <basetsd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <process.h>
#include <direct.h>
#include <winsock2.h>
#include <windows.h>
#include <wincon.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <conio.h>
#include <process.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <malloc.h>
#include <setjmp.h>
#include <direct.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>


// STL includes
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
using namespace std;   

#include <atlcomcli.h>
#include <objbase.h>


// Used for safe string manipulation
#include <strsafe.h>
#include "vss.h"


#pragma comment(lib,"atlsd.lib")



// Constructor
VSSClient::VSSClient()
{
    m_bCoInitializeCalled = false;
    m_bCoInitializeSecurityCalled = false;
    m_dwContext = 0; // VSS_CTX_BACKUP;
    m_bDuringRestore = false;
    m_bBackupIsInitialized = false;
    m_pVssObject = NULL;
    m_pVectorWriterStates = new vector<int>;
    m_pVectorWriterInfo = new vector<string>;
    m_uidCurrentSnapshotSet = GUID_NULL;
    memset(m_wszUniqueVolumeName,0, sizeof(m_wszUniqueVolumeName));
    memset(m_szShadowCopyName,0, sizeof(m_szShadowCopyName));
}

// Destructor
VSSClient::~VSSClient()
{
   // Release the IVssBackupComponents interface 
   // WARNING: this must be done BEFORE calling CoUninitialize()
   if (m_pVssObject) {
      m_pVssObject->Release();
      m_pVssObject = NULL;
   }

   if (m_pVectorWriterStates)
      delete (m_pVectorWriterStates);

   if (m_pVectorWriterInfo)
      delete (m_pVectorWriterInfo);

   // Call CoUninitialize if the CoInitialize was performed sucesfully
   if (m_bCoInitializeCalled)
      CoUninitialize();
}

BOOL VSSClient::InitializeForBackup()
{
    //return Initialize (VSS_CTX_BACKUP);
   return Initialize(0);
}




BOOL VSSClient::GetShadowPath(const char *szFilePath, char *szShadowPath, int nBuflen)
{
   if (!m_bBackupIsInitialized)
      return FALSE;

   /* check for valid pathname */
   BOOL bIsValidName;
   
   bIsValidName = strlen(szFilePath) > 3;
   if (bIsValidName)
      bIsValidName &= isalpha (szFilePath[0]) &&
                      szFilePath[1]==':' && 
                      szFilePath[2] == '\\';

   if (bIsValidName) {
      int nDriveIndex = toupper(szFilePath[0])-'A';
      if (m_szShadowCopyName[nDriveIndex][0] != 0) {
         strncpy(szShadowPath, m_szShadowCopyName[nDriveIndex], nBuflen);
         nBuflen -= (int)strlen(m_szShadowCopyName[nDriveIndex]);
         strncat(szShadowPath, szFilePath+2, nBuflen);
         return TRUE;
      }
   }
   
   strncpy(szShadowPath,  szFilePath, nBuflen);
   errno = EINVAL;
   return FALSE;   
}


const size_t VSSClient::GetWriterCount()
{
   vector<int>* pV = (vector<int>*) m_pVectorWriterStates;
   return pV->size();
}

const char* VSSClient::GetWriterInfo(size_t nIndex)
{
   vector<string>* pV = (vector<string>*) m_pVectorWriterInfo;   
   return pV->at(nIndex).c_str();
}


const int VSSClient::GetWriterState(size_t nIndex)
{
   vector<int>* pV = (vector<int>*) m_pVectorWriterStates;   
   return pV->at(nIndex);
}
