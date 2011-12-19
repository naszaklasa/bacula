/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2005-2010 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
//                              -*- Mode: C++ -*-
// vss.cpp -- Interface to Volume Shadow Copies (VSS)
//
// Copyright transferred from MATRIX-Computer GmbH to
//   Kern Sibbald by express permission.
//
// Author          : Thorsten Engel
// Created On      : Fri May 06 21:44:00 2005


#ifdef WIN32_VSS

#include "bacula.h"
#include "jcr.h"

#undef setlocale

// STL includes
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
using namespace std;

#include "ms_atl.h"
#include <objbase.h>

/* 
 * Kludges to get Vista code to compile.             
 *  KES - June 2007
 */
#define __in  IN
#define __out OUT
#define __RPC_unique_pointer
#define __RPC_string
#define __RPC__out_ecount_part(x, y)
#define __RPC__deref_inout_opt
#define __RPC__out

#if !defined(ENABLE_NLS)
#define setlocale(p, d)
#endif

#ifdef HAVE_STRSAFE_H
// Used for safe string manipulation
#include <strsafe.h>
#endif

BOOL VSSPathConvert(const char *szFilePath, char *szShadowPath, int nBuflen);
BOOL VSSPathConvertW(const wchar_t *szFilePath, wchar_t *szShadowPath, int nBuflen);

#ifdef HAVE_MINGW
class IXMLDOMDocument;
#endif

/* Reduce compiler warnings from Windows vss code */
#undef uuid
#define uuid(x)

#ifdef B_VSS_XP
   #define VSSClientGeneric VSSClientXP
   
   #include "inc/WinXP/vss.h"
   #include "inc/WinXP/vswriter.h"
   #include "inc/WinXP/vsbackup.h"

#endif

#ifdef B_VSS_W2K3
   #define VSSClientGeneric VSSClient2003
   
   #include "inc/Win2003/vss.h"
   #include "inc/Win2003/vswriter.h"
   #include "inc/Win2003/vsbackup.h"
#endif

#ifdef B_VSS_VISTA
   #define VSSClientGeneric VSSClientVista

   #include "inc/Win2003/vss.h"
   #include "inc/Win2003/vswriter.h"
   #include "inc/Win2003/vsbackup.h"
#endif
   
/* In VSSAPI.DLL */
typedef HRESULT (STDAPICALLTYPE* t_CreateVssBackupComponents)(OUT IVssBackupComponents **);
typedef void (APIENTRY* t_VssFreeSnapshotProperties)(IN VSS_SNAPSHOT_PROP*);
   
static t_CreateVssBackupComponents p_CreateVssBackupComponents = NULL;
static t_VssFreeSnapshotProperties p_VssFreeSnapshotProperties = NULL;



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
    if (path.length() <= 0) {
       return L"";
    }

    // Add the backslash termination, if needed
    path = AppendBackslash(path);

    // Get the root path of the volume
    wchar_t volumeRootPath[MAX_PATH];
    wchar_t volumeName[MAX_PATH];
    wchar_t volumeUniqueName[MAX_PATH];

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
    case value: return (GEN_MAKE_W(#value));


// Convert a writer status into a string
inline const wchar_t* GetStringFromWriterStatus(VSS_WRITER_STATE eWriterStatus)
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
        return L"Error or Undefined";
    }
}

// Constructor

#ifdef HAVE_VSS64
/* 64 bit entrypoint name */
#define VSSVBACK_ENTRY "?CreateVssBackupComponents@@YAJPEAPEAVIVssBackupComponents@@@Z"
#else
/* 32 bit entrypoint name */
#define VSSVBACK_ENTRY "?CreateVssBackupComponents@@YGJPAPAVIVssBackupComponents@@@Z"
#endif

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
bool VSSClientGeneric::Initialize(DWORD dwContext, bool bDuringRestore, bool (*VssInitCallback)(JCR *, int))
{
   CComPtr<IVssAsync>  pAsync1;
   VSS_BACKUP_TYPE backup_type;

   if (!(p_CreateVssBackupComponents && p_VssFreeSnapshotProperties)) {
      Dmsg2(0, "VSSClientGeneric::Initialize: p_CreateVssBackupComponents = 0x%08X, p_VssFreeSnapshotProperties = 0x%08X\n", p_CreateVssBackupComponents, p_VssFreeSnapshotProperties);
      errno = ENOSYS;
      return false;
   }

   HRESULT hr;
   // Initialize COM
   if (!m_bCoInitializeCalled) {
      hr = CoInitialize(NULL);
      if (FAILED(hr)) {
         Dmsg1(0, "VSSClientGeneric::Initialize: CoInitialize returned 0x%08X\n", hr);
         errno = b_errno_win32;
         return false;
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
         Dmsg1(0, "VSSClientGeneric::Initialize: CoInitializeSecurity returned 0x%08X\n", hr);
         errno = b_errno_win32;
         return false;
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
      berrno be;
      Dmsg2(0, "VSSClientGeneric::Initialize: CreateVssBackupComponents returned 0x%08X. ERR=%s\n",
            hr, be.bstrerror(b_errno_win32));
      errno = b_errno_win32;
      return false;
   }

#if   defined(B_VSS_W2K3) || defined(B_VSS_VISTA)
   if (dwContext != VSS_CTX_BACKUP) {
      hr = ((IVssBackupComponents*) m_pVssObject)->SetContext(dwContext);
      if (FAILED(hr)) {
         Dmsg1(0, "VSSClientGeneric::Initialize: IVssBackupComponents->SetContext returned 0x%08X\n", hr);
         errno = b_errno_win32;
         return false;
      }
   }
#endif

   if (!bDuringRestore) {
      // 1. InitializeForBackup
      hr = ((IVssBackupComponents*) m_pVssObject)->InitializeForBackup();
      if (FAILED(hr)) {
         Dmsg1(0, "VSSClientGeneric::Initialize: IVssBackupComponents->InitializeForBackup returned 0x%08X\n", hr);
         errno = b_errno_win32; 
         return false;
      }
 
      // 2. SetBackupState
      switch (m_jcr->getJobLevel())
      {
      case L_FULL:
         backup_type = VSS_BT_FULL;
         break;
      case L_DIFFERENTIAL:
         backup_type = VSS_BT_DIFFERENTIAL;
         break;
      case L_INCREMENTAL:
         backup_type = VSS_BT_INCREMENTAL;
         break;
      default:
         Dmsg1(0, "VSSClientGeneric::Initialize: unknown backup level %d\n", m_jcr->getJobLevel());
         backup_type = VSS_BT_FULL;
         break;
      }
      hr = ((IVssBackupComponents*) m_pVssObject)->SetBackupState(true, true, backup_type, false);
      if (FAILED(hr)) {
         Dmsg1(0, "VSSClientGeneric::Initialize: IVssBackupComponents->SetBackupState returned 0x%08X\n", hr);
         errno = b_errno_win32;
         return false;
      }

      // 3. GatherWriterMetaData
      hr = ((IVssBackupComponents*) m_pVssObject)->GatherWriterMetadata(&pAsync1.p);
      if (FAILED(hr)) {
         Dmsg1(0, "VSSClientGeneric::Initialize: IVssBackupComponents->GatherWriterMetadata returned 0x%08X\n", hr);
         errno = b_errno_win32;
         return false;
      }
      // Waits for the async operation to finish and checks the result
      WaitAndCheckForAsyncOperation(pAsync1.p);
   } else {
 
   /*
    * Initialize for restore
    */

      HRESULT hr;

#if 0
      WCHAR *xml;
      int fd;
      struct stat stat;
      /* obviously this is just temporary - the xml should come from somewhere like the catalog */
      fd = open("C:\\james.xml", O_RDONLY);
      Dmsg1(0, "fd = %d\n", fd);
      fstat(fd, &stat);
      Dmsg1(0, "size = %d\n", stat.st_size);
      xml = new WCHAR[stat.st_size / sizeof(WCHAR) + 1];
      read(fd, xml, stat.st_size);
      close(fd);
      xml[stat.st_size / sizeof(WCHAR)] = 0;
#endif

      // 1. InitializeForRestore
      hr = ((IVssBackupComponents*) m_pVssObject)->InitializeForRestore(m_metadata);
      if (FAILED(hr)) {
         Dmsg1(0, "VSSClientGeneric::Initialize: IVssBackupComponents->InitializeForRestore returned 0x%08X\n", hr);
         errno = b_errno_win32;
         return false;
      }
      VssInitCallback(m_jcr, VSS_INIT_RESTORE_AFTER_INIT);

      // 2. GatherWriterMetaData
      hr = ((IVssBackupComponents*) m_pVssObject)->GatherWriterMetadata(&pAsync1.p);
      if (FAILED(hr)) {
         Dmsg1(0, "VSSClientGeneric::Initialize: IVssBackupComponents->GatherWriterMetadata returned 0x%08X\n", hr);
         errno = b_errno_win32;
         return false;
      }
      WaitAndCheckForAsyncOperation(pAsync1.p);
      VssInitCallback(m_jcr, VSS_INIT_RESTORE_AFTER_GATHER);

      // 3. PreRestore
      hr = ((IVssBackupComponents*) m_pVssObject)->PreRestore(&pAsync1.p);
      if (FAILED(hr)) {
         Dmsg1(0, "VSSClientGeneric::Initialize: IVssBackupComponents->PreRestore returned 0x%08X\n", hr);
         errno = b_errno_win32;
         return false;
      }
      WaitAndCheckForAsyncOperation(pAsync1.p);
      /* get latest info about writer status */
      if (!CheckWriterStatus()) {
         Dmsg0(0, "VSSClientGeneric::InitializePostPlugin: Failed to CheckWriterstatus\n");
         errno = b_errno_win32;
         return false;
      }
   }

   // We are during restore now?
   m_bDuringRestore = bDuringRestore;

   // Keep the context
   m_dwContext = dwContext;

   return true;
}

bool VSSClientGeneric::WaitAndCheckForAsyncOperation(IVssAsync* pAsync)
{
   // Wait until the async operation finishes
   // unfortunately we can't use a timeout here yet.
   // the interface would allow it on W2k3,
   // but it is not implemented yet....

   HRESULT hr;

   // Check the result of the asynchronous operation
   HRESULT hrReturned = S_OK;

   int timeout = 600; // 10 minutes....

   int queryErrors = 0;
   do {
      if (hrReturned != S_OK) 
         Sleep(1000);
   
      hrReturned = S_OK;
      hr = pAsync->QueryStatus(&hrReturned, NULL);
   
      if (FAILED(hr)) 
         queryErrors++;
   } while ((timeout-- > 0) && (hrReturned == VSS_S_ASYNC_PENDING));

   if (hrReturned == VSS_S_ASYNC_FINISHED)
      return true;

   
#ifdef xDEBUG 
   // Check if the async operation succeeded...
   if(hrReturned != VSS_S_ASYNC_FINISHED) {   
      wchar_t *pwszBuffer = NULL;
      /* I don't see the usefulness of the following -- KES */
      FormatMessageW(
         FORMAT_MESSAGE_ALLOCATE_BUFFER 
         | FORMAT_MESSAGE_FROM_SYSTEM 
         | FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL, hrReturned, 
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
         (LPWSTR)&pwszBuffer, 0, NULL);

      LocalFree(pwszBuffer);         
      errno = b_errno_win32;
   }
#endif

   return false;
}

bool VSSClientGeneric::CreateSnapshots(char* szDriveLetters)
{
   /* szDriveLetters contains all drive letters in uppercase */
   /* if a drive can not being added, it's converted to lowercase in szDriveLetters */
   /* http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vss/base/ivssbackupcomponents_startsnapshotset.asp */
   
   if (!m_pVssObject || m_bBackupIsInitialized) {
      errno = ENOSYS;
      return false;  
   }

   m_uidCurrentSnapshotSet = GUID_NULL;

   IVssBackupComponents *pVss = (IVssBackupComponents*)m_pVssObject;

   /* startSnapshotSet */

   pVss->StartSnapshotSet(&m_uidCurrentSnapshotSet);

   /* AddToSnapshotSet */

   wchar_t szDrive[3];
   szDrive[1] = ':';
   szDrive[2] = 0;

   wstring volume;

   CComPtr<IVssAsync>  pAsync1;
   CComPtr<IVssAsync>  pAsync2;   
   VSS_ID pid;

   for (size_t i=0; i < strlen (szDriveLetters); i++) {
      szDrive[0] = szDriveLetters[i];
      volume = GetUniqueVolumeNameForPath(szDrive);
      // store uniquevolumname
      if (SUCCEEDED(pVss->AddToSnapshotSet((LPWSTR)volume.c_str(), GUID_NULL, &pid))) {
         wcsncpy (m_wszUniqueVolumeName[szDriveLetters[i]-'A'], (LPWSTR) volume.c_str(), MAX_PATH);
      } else {            
         szDriveLetters[i] = tolower (szDriveLetters[i]);               
      }
   }

   /* PrepareForBackup */
   if (FAILED(pVss->PrepareForBackup(&pAsync1.p))) {      
      errno = b_errno_win32;
      return false;   
   }
   
   // Waits for the async operation to finish and checks the result
   WaitAndCheckForAsyncOperation(pAsync1.p);

   /* get latest info about writer status */
   if (!CheckWriterStatus()) {
      errno = b_errno_win32;
      return false;
   }

   /* DoSnapShotSet */   
   if (FAILED(pVss->DoSnapshotSet(&pAsync2.p))) {      
      errno = b_errno_win32;
      return false;   
   }

   // Waits for the async operation to finish and checks the result
   WaitAndCheckForAsyncOperation(pAsync2.p); 
   
   /* query snapshot info */   
   QuerySnapshotSet(m_uidCurrentSnapshotSet);

   SetVSSPathConvert(VSSPathConvert, VSSPathConvertW);

   m_bBackupIsInitialized = true;

   return true;
}

bool VSSClientGeneric::CloseBackup()
{
   bool bRet = false;
   HRESULT hr;
   BSTR xml;

   if (!m_pVssObject)
      errno = ENOSYS;
   else {
      IVssBackupComponents* pVss = (IVssBackupComponents*) m_pVssObject;
      CComPtr<IVssAsync>  pAsync;

      SetVSSPathConvert(NULL, NULL);

      m_bBackupIsInitialized = false;

      hr = pVss->SaveAsXML(&xml);
      if (hr == ERROR_SUCCESS)
         m_metadata = xml;
      else
         m_metadata = NULL;
#if 0
{
   HRESULT hr;
   BSTR xml;
   int fd;

   hr = pVss->SaveAsXML(&xml);
   fd = open("C:\\james.xml", O_CREAT | O_WRONLY | O_TRUNC, 0777);
   write(fd, xml, wcslen(xml) * sizeof(WCHAR));
   close(fd);
}
#endif
      if (SUCCEEDED(pVss->BackupComplete(&pAsync.p))) {
         // Waits for the async operation to finish and checks the result
         WaitAndCheckForAsyncOperation(pAsync.p);
         bRet = true;     
      } else {
         errno = b_errno_win32;
         pVss->AbortBackup();
      }

      /* get latest info about writer status */
      CheckWriterStatus();

      if (m_uidCurrentSnapshotSet != GUID_NULL) {
         VSS_ID idNonDeletedSnapshotID = GUID_NULL;
         LONG lSnapshots;

         pVss->DeleteSnapshots(
            m_uidCurrentSnapshotSet, 
            VSS_OBJECT_SNAPSHOT_SET,
            false,
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

WCHAR *VSSClientGeneric::GetMetadata()
{
   return m_metadata;
}

bool VSSClientGeneric::CloseRestore()
{
   HRESULT hr;
   IVssBackupComponents* pVss = (IVssBackupComponents*) m_pVssObject;
   CComPtr<IVssAsync> pAsync;

   if (!pVss)
   {
      errno = ENOSYS;
      return false;
   }
   if (SUCCEEDED(hr = pVss->PostRestore(&pAsync.p))) {
      // Waits for the async operation to finish and checks the result
      WaitAndCheckForAsyncOperation(pAsync.p);
      /* get latest info about writer status */
      if (!CheckWriterStatus()) {
         errno = b_errno_win32;
         return false;
      }
   } else {
      errno = b_errno_win32;
      return false;
   }
   return true;
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
         (IVssEnumObject**)(&pIEnumSnapshots) );    

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
      hr = (pIEnumSnapshots.p)->Next( 1, &Prop, &ulFetched );

      // We reached the end of list
      if (ulFetched == 0)
         break;

      // Print the shadow copy (if not filtered out)
      if (Snap.m_SnapshotSetId == snapshotSetID)  {
         for (int ch='A'-'A';ch<='Z'-'A';ch++) {
            if (wcscmp(Snap.m_pwszOriginalVolumeName, m_wszUniqueVolumeName[ch]) == 0) {       
               wcsncpy(m_szShadowCopyName[ch],Snap.m_pwszSnapshotDeviceObject, MAX_PATH-1);               
               break;
            }
         }
      }
      p_VssFreeSnapshotProperties(&Snap);
   }
   errno = 0;
}

// Check the status for all selected writers
bool VSSClientGeneric::CheckWriterStatus()
{
    /* 
    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vss/base/ivssbackupcomponents_startsnapshotset.asp
    */
    IVssBackupComponents* pVss = (IVssBackupComponents*) m_pVssObject;
    DestroyWriterInfo();

    // Gather writer status to detect potential errors
    CComPtr<IVssAsync>  pAsync;
    
    HRESULT hr = pVss->GatherWriterStatus(&pAsync.p);
    if (FAILED(hr)) {
       errno = b_errno_win32;
       return false;
    } 

    // Waits for the async operation to finish and checks the result
    WaitAndCheckForAsyncOperation(pAsync.p);
      
    unsigned cWriters = 0;

    hr = pVss->GetWriterStatusCount(&cWriters);
    if (FAILED(hr)) {
       errno = b_errno_win32;
       return false;
    }

    int nState;
    
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
            /* unknown */            
            nState = 0;
        }
        else {            
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
    #if  defined(B_VSS_W2K3) || defined(B_VSS_VISTA)
            case VSS_WS_FAILED_AT_BACKUPSHUTDOWN:
    #endif
                /* failed */                
                nState = -1;
                break;

            default:
                /* ok */
                nState = 1;
            }
        }
        /* store text info */
        char str[1000];
        char szBuf[200];        
        bstrncpy(str, "\"", sizeof(str));
        wchar_2_UTF8(szBuf, bstrWriterName.p, sizeof(szBuf));
        bstrncat(str, szBuf, sizeof(str));
        bstrncat(str, "\", State: 0x", sizeof(str));
        itoa(eWriterStatus, szBuf, sizeof(szBuf));
        bstrncat(str, szBuf, sizeof(str));
        bstrncat(str, " (", sizeof(str));
        wchar_2_UTF8(szBuf, GetStringFromWriterStatus(eWriterStatus), sizeof(szBuf));
        bstrncat(str, szBuf, sizeof(str));
        bstrncat(str, ")", sizeof(str));

        AppendWriterInfo(nState, (const char *)str);     
    }

    hr = pVss->FreeWriterStatus();

    if (FAILED(hr)) {
        errno = b_errno_win32;
        return false;
    } 

    errno = 0;
    return true;
}

#endif /* WIN32_VSS */
