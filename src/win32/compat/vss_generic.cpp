//                              -*- Mode: C++ -*-
// vss.cpp -- Interface to Volume Shadow Copies (VSS)
//
// Copyright transferred from MATRIX-Computer GmbH to
//   Kern Sibbald by express permission.
//
//  Copyright (C) 2005-2006 Kern Sibbald
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
#include <sstream>
#include <fstream>
using namespace std;   

#include <atlcomcli.h>
#include <objbase.h>

// Used for safe string manipulation
#include <strsafe.h>

#include "../../lib/winapi.h"

#ifdef B_VSS_XP
   #pragma message("compile VSS for Windows XP")   
   #define VSSClientGeneric VSSClientXP
   // wait is not available under XP...
   #define VSS_TIMEOUT

   #include "vss/inc/WinXP/vss.h"
   #include "vss/inc/WinXP/vswriter.h"
   #include "vss/inc/WinXP/vsbackup.h"
   
   /* In VSSAPI.DLL */
   typedef HRESULT (STDAPICALLTYPE* t_CreateVssBackupComponents)(OUT IVssBackupComponents **);
   typedef void (APIENTRY* t_VssFreeSnapshotProperties)(IN VSS_SNAPSHOT_PROP*);
   
   static t_CreateVssBackupComponents p_CreateVssBackupComponents = NULL;
   static t_VssFreeSnapshotProperties p_VssFreeSnapshotProperties = NULL;

   #define VSSVBACK_ENTRY "?CreateVssBackupComponents@@YGJPAPAVIVssBackupComponents@@@Z"
#endif

#ifdef B_VSS_W2K3
   #pragma message("compile VSS for Windows 2003")
   #define VSSClientGeneric VSSClient2003
   // wait x ms for a VSS asynchronous operation (-1 = infinite)
   // unfortunately, it doesn't work, so do not set timeout
   #define VSS_TIMEOUT

   #include "vss/inc/Win2003/vss.h"
   #include "vss/inc/Win2003/vswriter.h"
   #include "vss/inc/Win2003/vsbackup.h"
   
   /* In VSSAPI.DLL */
   typedef HRESULT (STDAPICALLTYPE* t_CreateVssBackupComponents)(OUT IVssBackupComponents **);
   typedef void (APIENTRY* t_VssFreeSnapshotProperties)(IN VSS_SNAPSHOT_PROP*);
   
   static t_CreateVssBackupComponents p_CreateVssBackupComponents = NULL;
   static t_VssFreeSnapshotProperties p_VssFreeSnapshotProperties = NULL;

   #define VSSVBACK_ENTRY "?CreateVssBackupComponents@@YGJPAPAVIVssBackupComponents@@@Z"
#endif

#include "vss.h"

/*  
 *
 * some helper functions 
 *
 *
 */

// Append a backslash to the current string 
inline wstring AppendBackslash(wstring str)
{
    if (str.length() == 0)
        return wstring(L"\\");
    if (str[str.length() - 1] == L'\\')
        return str;
    return str.append(L"\\");
}

// Get the unique volume name for the given path
inline wstring GetUniqueVolumeNameForPath(wstring path)
{
    _ASSERTE(path.length() > 0);    

    // Add the backslash termination, if needed
    path = AppendBackslash(path);

    // Get the root path of the volume
    WCHAR volumeRootPath[MAX_PATH];
    WCHAR volumeName[MAX_PATH];
    WCHAR volumeUniqueName[MAX_PATH];

    if (!p_GetVolumePathNameW || !p_GetVolumePathNameW((LPCWSTR)path.c_str(), volumeRootPath, MAX_PATH))
      return L"";
    
    // Get the volume name alias (might be different from the unique volume name in rare cases)
    if (!p_GetVolumeNameForVolumeMountPointW || !p_GetVolumeNameForVolumeMountPointW(volumeRootPath, volumeName, MAX_PATH))
       return L"";
    
    // Get the unique volume name    
    if (!p_GetVolumeNameForVolumeMountPointW(volumeName, volumeUniqueName, MAX_PATH))
       return L"";
    
    return volumeUniqueName;
}


// Helper macro for quick treatment of case statements for error codes
#define GEN_MERGE(A, B) A##B
#define GEN_MAKE_W(A) GEN_MERGE(L, A)

#define CHECK_CASE_FOR_CONSTANT(value)                      \
    case value: return wstring(GEN_MAKE_W(#value));


// Convert a writer status into a string
inline wstring GetStringFromWriterStatus(VSS_WRITER_STATE eWriterStatus)
{
    switch (eWriterStatus) {
    CHECK_CASE_FOR_CONSTANT(VSS_WS_STABLE);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_WAITING_FOR_FREEZE);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_WAITING_FOR_THAW);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_WAITING_FOR_POST_SNAPSHOT);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_WAITING_FOR_BACKUP_COMPLETE);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_IDENTIFY);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_PREPARE_BACKUP);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_PREPARE_SNAPSHOT);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_FREEZE);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_THAW);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_POST_SNAPSHOT);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_BACKUP_COMPLETE);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_PRE_RESTORE);
    CHECK_CASE_FOR_CONSTANT(VSS_WS_FAILED_AT_POST_RESTORE);

    default:
        return wstring(L"Error or Undefined");
    }
}

// Constructor

VSSClientGeneric::VSSClientGeneric()
{
   m_hLib = LoadLibraryA("VSSAPI.DLL");
   if (m_hLib) {      
      p_CreateVssBackupComponents = (t_CreateVssBackupComponents)
         GetProcAddress(m_hLib, VSSVBACK_ENTRY);
                                 
      p_VssFreeSnapshotProperties = (t_VssFreeSnapshotProperties)
          GetProcAddress(m_hLib, "VssFreeSnapshotProperties");      
   } 
}



// Destructor
VSSClientGeneric::~VSSClientGeneric()
{
   if (m_hLib)
      FreeLibrary(m_hLib);
}

// Initialize the COM infrastructure and the internal pointers
BOOL VSSClientGeneric::Initialize(DWORD dwContext, BOOL bDuringRestore)
{
   if (!(p_CreateVssBackupComponents && p_VssFreeSnapshotProperties)) {
      errno = ENOSYS;
      return FALSE;
   }

   HRESULT hr;
   // Initialize COM 
   if (!m_bCoInitializeCalled)  {
      if (FAILED(CoInitialize(NULL))) {
         errno = b_errno_win32;
         return FALSE;
      }
      m_bCoInitializeCalled = true;
   }

   // Initialize COM security
   if (!m_bCoInitializeSecurityCalled) {
      hr =
         CoInitializeSecurity(
         NULL,                           //  Allow *all* VSS writers to communicate back!
         -1,                             //  Default COM authentication service
         NULL,                           //  Default COM authorization service
         NULL,                           //  reserved parameter
         RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  //  Strongest COM authentication level
         RPC_C_IMP_LEVEL_IDENTIFY,       //  Minimal impersonation abilities 
         NULL,                           //  Default COM authentication settings
         EOAC_NONE,                      //  No special options
         NULL                            //  Reserved parameter
         );

      if (FAILED(hr)) {
         errno = b_errno_win32;
         return FALSE;
      }
      m_bCoInitializeSecurityCalled = true;      
   }
   
   // Release the IVssBackupComponents interface 
   if (m_pVssObject) {
      m_pVssObject->Release();
      m_pVssObject = NULL;
   }

   // Create the internal backup components object
   hr = p_CreateVssBackupComponents((IVssBackupComponents**) &m_pVssObject);
   if (FAILED(hr)) {
      errno = b_errno_win32;
      return FALSE;
   }

#ifdef B_VSS_W2K3
   if (dwContext != VSS_CTX_BACKUP) {
      hr = ((IVssBackupComponents*) m_pVssObject)->SetContext(dwContext);
      if (FAILED(hr)) {
         errno = b_errno_win32;
         return FALSE;
      }
   }
#endif

   // We are during restore now?
   m_bDuringRestore = bDuringRestore;

   // Keep the context
   m_dwContext = dwContext;

   return TRUE;
}


void VSSClientGeneric::WaitAndCheckForAsyncOperation(IVssAsync* pAsync)
{
    // Wait until the async operation finishes
    // unfortunately we can't use a timeout here yet.
    // the interface would allow it on W2k3, but it is not implemented yet....
    HRESULT hr = pAsync->Wait(VSS_TIMEOUT);

    // Check the result of the asynchronous operation
    HRESULT hrReturned = S_OK;
    hr = pAsync->QueryStatus(&hrReturned, NULL);

    // Check if the async operation succeeded...
    if(FAILED(hrReturned)) {
      PWCHAR pwszBuffer = NULL;
      DWORD dwRet = ::FormatMessageW(
         FORMAT_MESSAGE_ALLOCATE_BUFFER 
         | FORMAT_MESSAGE_FROM_SYSTEM 
         | FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL, hrReturned, 
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
         (LPWSTR)&pwszBuffer, 0, NULL);

      // No message found for this error. Just return <Unknown>
      if (dwRet != 0) {
         LocalFree(pwszBuffer);         
      }
    }
}

BOOL VSSClientGeneric::CreateSnapshots(char* szDriveLetters)
{
   /* szDriveLetters contains all drive letters in uppercase */
   /* if a drive can not being added, it's converted to lowercase in szDriveLetters */
   /* http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vss/base/ivssbackupcomponents_startsnapshotset.asp */
   
   if (!m_pVssObject || m_bBackupIsInitialized) {
      errno = ENOSYS;
      return FALSE;  
   }

   m_uidCurrentSnapshotSet = GUID_NULL;

   IVssBackupComponents *pVss = (IVssBackupComponents*)m_pVssObject;

   // 1. InitializeForBackup
   HRESULT hr = pVss->InitializeForBackup();
   if (FAILED(hr)) {
      errno = b_errno_win32;
      return FALSE;
   }
   
   // 2. SetBackupState
   hr = pVss->SetBackupState(true, true, VSS_BT_FULL, false);
   if (FAILED(hr)) {
      errno = b_errno_win32;
      return FALSE;
   }

   CComPtr<IVssAsync>  pAsync1;
   CComPtr<IVssAsync>  pAsync2;   
   CComPtr<IVssAsync>  pAsync3;   
   VSS_ID pid;

   // 3. GatherWriterMetaData
   hr = pVss->GatherWriterMetadata(&pAsync3);
   if (FAILED(hr)) {
      errno = b_errno_win32;
      return FALSE;
   }

   // Waits for the async operation to finish and checks the result
   WaitAndCheckForAsyncOperation(pAsync3);

   CheckWriterStatus();

   // 4. startSnapshotSet

   pVss->StartSnapshotSet(&m_uidCurrentSnapshotSet);

   // 4. AddToSnapshotSet

   WCHAR szDrive[3];
   szDrive[1] = ':';
   szDrive[2] = 0;

   wstring volume;

   for (size_t i=0; i < strlen (szDriveLetters); i++) {
      szDrive[0] = szDriveLetters[i];
      volume = GetUniqueVolumeNameForPath(szDrive);
      // store uniquevolumname
      if (SUCCEEDED(pVss->AddToSnapshotSet((LPWSTR)volume.c_str(), GUID_NULL, &pid)))
         wcsncpy (m_wszUniqueVolumeName[szDriveLetters[i]-'A'], (LPWSTR) volume.c_str(), MAX_PATH);
      else
         szDriveLetters[i] = tolower (szDriveLetters[i]);               
   }

   // 5. PrepareForBackup
   pVss->PrepareForBackup(&pAsync1);
   
   // Waits for the async operation to finish and checks the result
   WaitAndCheckForAsyncOperation(pAsync1);

   // 6. DoSnapShotSet
   pVss->DoSnapshotSet(&pAsync2);

   // Waits for the async operation to finish and checks the result
   WaitAndCheckForAsyncOperation(pAsync2); 
   
   /* query snapshot info */
   QuerySnapshotSet(m_uidCurrentSnapshotSet);

   m_bBackupIsInitialized = true;

   return TRUE;
}

BOOL VSSClientGeneric::CloseBackup()
{
   BOOL bRet = FALSE;
   if (!m_pVssObject)
      errno = ENOSYS;
   else {
      IVssBackupComponents* pVss = (IVssBackupComponents*) m_pVssObject;
      CComPtr<IVssAsync>  pAsync;
      
      m_bBackupIsInitialized = false;

      if (SUCCEEDED(pVss->BackupComplete(&pAsync))) {
         // Waits for the async operation to finish and checks the result
         WaitAndCheckForAsyncOperation(pAsync);
         bRet = TRUE;     
      } else {
         errno = b_errno_win32;
         pVss->AbortBackup();
      }

      if (m_uidCurrentSnapshotSet != GUID_NULL) {
         VSS_ID idNonDeletedSnapshotID = GUID_NULL;
         LONG lSnapshots;

         pVss->DeleteSnapshots(
            m_uidCurrentSnapshotSet, 
            VSS_OBJECT_SNAPSHOT_SET,
            FALSE,
            &lSnapshots,
            &idNonDeletedSnapshotID);

         m_uidCurrentSnapshotSet = GUID_NULL;
      }

      pVss->Release();
      m_pVssObject = NULL;
   }

   // Call CoUninitialize if the CoInitialize was performed sucesfully
   if (m_bCoInitializeCalled) {
      CoUninitialize();
      m_bCoInitializeCalled = false;
   }

   return bRet;
}

// Query all the shadow copies in the given set
void VSSClientGeneric::QuerySnapshotSet(GUID snapshotSetID)
{   
   if (!(p_CreateVssBackupComponents && p_VssFreeSnapshotProperties)) {
      errno = ENOSYS;
      return;
   }

   memset (m_szShadowCopyName,0,sizeof (m_szShadowCopyName));
   
   if (snapshotSetID == GUID_NULL || m_pVssObject == NULL) {
      errno = ENOSYS;
      return;
   }

   IVssBackupComponents* pVss = (IVssBackupComponents*) m_pVssObject;
               
   // Get list all shadow copies. 
   CComPtr<IVssEnumObject> pIEnumSnapshots;
   HRESULT hr = pVss->Query( GUID_NULL, 
         VSS_OBJECT_NONE, 
         VSS_OBJECT_SNAPSHOT, 
         &pIEnumSnapshots );    

   // If there are no shadow copies, just return
   if (FAILED(hr)) {
      errno = b_errno_win32;
      return;   
   }

   // Enumerate all shadow copies. 
   VSS_OBJECT_PROP Prop;
   VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
   
   while (true) {
      // Get the next element
      ULONG ulFetched;
      hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );

      // We reached the end of list
      if (ulFetched == 0)
         break;

      // Print the shadow copy (if not filtered out)
      if (Snap.m_SnapshotSetId == snapshotSetID)  {
         for (char ch='A'-'A';ch<='Z'-'A';ch++) {
            if (wcscmp(Snap.m_pwszOriginalVolumeName, m_wszUniqueVolumeName[ch]) == 0) {               
               WideCharToMultiByte(CP_UTF8,0,Snap.m_pwszSnapshotDeviceObject,-1,m_szShadowCopyName[ch],MAX_PATH*2,NULL,NULL);               
               break;
            }
         }
      }
      p_VssFreeSnapshotProperties(&Snap);
   }
   errno = 0;
}

// Check the status for all selected writers
BOOL VSSClientGeneric::CheckWriterStatus()
{
    IVssBackupComponents* pVss = (IVssBackupComponents*) m_pVssObject;
    vector<int>* pVWriterStates = (vector<int>*) m_pVectorWriterStates;
    vector<string>* pVWriterInfo = (vector<string>*) m_pVectorWriterInfo;   

    pVWriterStates->clear();
    pVWriterInfo->clear();

    // Gather writer status to detect potential errors
    CComPtr<IVssAsync>  pAsync;
    
    HRESULT hr = pVss->GatherWriterStatus(&pAsync);
    if (FAILED(hr)) {
       errno = b_errno_win32;
       return FALSE;
    } 

    // Waits for the async operation to finish and checks the result
    WaitAndCheckForAsyncOperation(pAsync);
      
    unsigned cWriters = 0;

    hr = pVss->GetWriterStatusCount(&cWriters);
    if (FAILED(hr)) {
       errno = b_errno_win32;
       return FALSE;
    }
    
    // Enumerate each writer
    for (unsigned iWriter = 0; iWriter < cWriters; iWriter++) {
        VSS_ID idInstance = GUID_NULL;
        VSS_ID idWriter= GUID_NULL;
        VSS_WRITER_STATE eWriterStatus = VSS_WS_UNKNOWN;
        CComBSTR bstrWriterName;
        HRESULT hrWriterFailure = S_OK;

        // Get writer status
        hr = pVss->GetWriterStatus(iWriter,
                             &idInstance,
                             &idWriter,
                             &bstrWriterName,
                             &eWriterStatus,
                             &hrWriterFailure);
        if (FAILED(hr)) {
           continue;
        }
        
        // If the writer is in non-stable state, break
        switch(eWriterStatus) {
        case VSS_WS_FAILED_AT_IDENTIFY:
        case VSS_WS_FAILED_AT_PREPARE_BACKUP:
        case VSS_WS_FAILED_AT_PREPARE_SNAPSHOT:
        case VSS_WS_FAILED_AT_FREEZE:
        case VSS_WS_FAILED_AT_THAW:
        case VSS_WS_FAILED_AT_POST_SNAPSHOT:
        case VSS_WS_FAILED_AT_BACKUP_COMPLETE:
        case VSS_WS_FAILED_AT_PRE_RESTORE:
        case VSS_WS_FAILED_AT_POST_RESTORE:
#ifdef B_VSS_W2K3
        case VSS_WS_FAILED_AT_BACKUPSHUTDOWN:
#endif
           /* failed */
           pVWriterStates->push_back(-1);
           break;

        default:
           /* okay */
           pVWriterStates->push_back(1);                
        }

               
        stringstream osf;      
        osf << "\"" << CW2A(bstrWriterName) << "\", State: " << eWriterStatus << " (" << CW2A(GetStringFromWriterStatus(eWriterStatus).c_str()) << ")";
    
        pVWriterInfo->push_back(osf.str());
    }
    errno = 0;
    return TRUE;
}
