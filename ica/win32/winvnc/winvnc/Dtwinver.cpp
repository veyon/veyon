/*
Module : Dtwinver.cpp
Purpose: Implementation of a comprehensive function to perform OS version detection
Created: PJN / 11-05-1996
History: PJN / 24-02-1997 A number of updates including support for NT 3.1, 
                          single mode dos in Windows 95 and better Windows
                          version detecion under real mode dos.
         PJN / 13-09-1998 1.  Added explicit support for Windows 98 
                          2.  Updated documentation to use HTML. 
                          3.  Minor update to the web page describing it. 
         PJN / 22-06-1999 1.  UNICODE enabled the code.
                          2.  Removed need for the dwOSVersionInfoSize variable
                          3.  Added support for detecting Build Number of 95 and 98 from DOS code path.
                          4.  Now ships as standard with VC 5 workspace files
                          5.  Added explicit support for Windows 95 SP 1
                          6.  Added explicit support for Windows 95 OSR 2
                          7.  Added explicit support for Windows 98 Second Edition
                          8.  Added explicit support for Windows 2000
                          9.  Added explicit support for Windows CE
                          10. Added explicit support for Windows Terminal Server's
                          11. Added explicit support for NT Stand Alone Server's.
                          12. Added explicit support for NT Primary Domain Controller's
                          13. Added explicit support for NT Backup Domain Controller's
         PJN / 23-07-1999 Tested out support for Windows 98 SE, minor changes required
         PJN / 26-07-1999 Added explicit support for Windows 98 SP 1
         PJN / 28-07-1999 1. Fixed a problem when application is build in non-huge/large 
                          memory model in Win16
                          2. Added explicit support for returning NT and Win9x service pack information 
                          from Win32 and Win16 code paths
                          3. Updated test program to not bother reporting on any info which does not 
                          exist. e.g. if there is no service pack installed, then we don't bother 
                          displaying any info about service packs
                          4. Added explicit support for NT Enterprise Edition
         PJN / 30-06-2000 1. Added explicit support for Windows Millennium Edition
         PJN / 29-01-2001 1. Added explicit support for Windows XP (Whistler) Personal
                          2. Added explicit support for Windows XP (Whistler) Professional
                          3. Added explicit support for Windows XP (Whistler) Server
                          4. Added explicit support for Windows XP (Whistler) Advanced Server
                          5. Added explicit support for Windows XP (Whistler) Datacenter
                          6. Added explicit support for Windows XP (Whistler) 64 bit (all flavours)
                          7. Made all the code into a C++ class called COSVersion
                          8. Rewrote all the generic thunk code to be easier to follow
                          9. Generic think code now uses CallProcEx32W
                          10. Added explicit support for BackOffice Small Business Edition
                          11. Added explicit support for Terminal Services
                          12. 16 bit code path now can determine ProductSuite and ProductType type
                          thro additional generic thunk code
                          13. Provided a 64 bit test binary and make batch file (make64.bat) for 
                          those lucky enough to have an Itanium processor and a beta of 64 bit Windows XP (Whistler).
                          14. Provided a Embedded C++ workspace and X86 Release binary.
                          15. Updated copyright information           
         PJN / 10-02-2001 1. Updated function names etc following MS decision to call Whistler "Windows XP"
         PJN / 10-10-2001 1. Added code to 32 bit code path to detect if we are being run under 64 bit Windows. Also
                          updated the sample app to distinguish between emulated 64 bit and underlying 
                          64 bit.
                          2. Updated the sample app to call XP Server its proper name which will be "Windows.NET Server"
         PJN / 13-12-2001 1. Major upgrade. Now 16 bit DOS path can return as much information as native Win32 code. 
                          This is achieved by spawning off the Win32 utility WriteVer with a special command line option.
                          Please note that if you intend deploying dtwinver then you must now ship the writever.exe file
                          in addition to linking in the dtwinver code into your application. Also this utilty needs
                          to be in the path or current directory when the dtwinver code is executing. Thanks to Chaz Angell
                          for prompted me into finding a solution for this last major item missing in dtwinver.
         PJN / 30-12-2002 1. Provided an update CE workspace to work correctly in eMbedded Visual C++ v3. All build configurations
                          for eVC 3 have also been provided.
                          2. Optimized the usage of _tcscat and _tcscpy in the test app which comes with Dtwinver.cpp
                          3. OEM Info string and Platform Type string is now returned for CE code path
                          4. Fixed display of minor version number for example Windows .NET is version number v5.20 but
                          should be shown as 5.2 to be consistent with what the native ver command displays
                          5. Provided a new CE workspace to work correctly in eMbedded Visual C++ v4. All build configurations
                          for eVC 4 have also been provided.
         PJN / 08-10-2002 1. Now uses OSVERSIONINFOEX it possible in the Win32 or Win64 code paths. This provides for
                          more reliably detection of Windows XP Home Edition.
                          2. Renamed the functions which detect Windows .NET Server 2003. Also updated the test code which
                          prints out these names
                          3. Provided explicit support for Windows .NET Web Server
                          4. Fixed a bug in the display of the minor OS version number on Windows .NET Server.
                          5. Made the project for WriteVer a VC 5 project instead of VC 6 which it was up until now.
                          6. Reworked the internal function WhichNTProduct to use a constant input string parameter
                          7. Added explicit support for Windows NT / 2000 / XP Embedded
                          8. Added explicit support for detecting Terminal Services in remote admin mode
         PJN / 10-10-2002 1.  Fixed a problem where on Windows XP, the test program will include the text "(Standard Edition)"
                          2.  Added two variables to the OS_VERSION_INFO structure to report the minor service pack number
                          3.  Removed the OS_VERSION_INFO structure from the global namespace
                          4.  Removed all static member variables from the class
                          5.  Added a member variable to the OS_VERSION_INFO to return the various "suites" installed
                          6.  reduced the number of calls to WriteVer to 1 when called from dos code path.
                          7.  Completely reworked the internal WhichNTProduct method
                          8.  General tidy up of the header file
                          9.  Completely reworked the ValidateProductSuite method
                          10. Now only 1 call is made to WhichNTProduct throughout a single call to COSVersion::GetVersion
                          11. Now only 1 call is made to ValidateProductSuite throughout a single call to COSVersion::GetVersion
                          12. Fixed an unitialized variable problem in COSVersion::IsUnderlying64Bit
                          13. Changed "WhichNTProduct" method to "GetNTOSTypeFromRegistry"
                          14. Changed "ValidateProductSuite" method to "GetProductSuiteDetailsFromRegistry".
                          15. Now correctly reports on Terminal Services being in Remote Admin Mode on OS which do not
                          support calling GetVersionEx using an OSVERSIONINFOEX structure i.e any NT 4 install prior to SP6.
                          16. 16 bit Windows code path now reports as much NT information as the Win32 code path 
                          17. Fixed a bug in COSVersion::GetInfoBySpawingWriteVer which was failing if it encountered 
                          an empty CSD string. This was spotted on Windows .NET Server which since it is in beta still
                          (as of October 2002) does not have any service pack!.
         PJN / 10-01-2003 1. Update to support MS deciding to change the name of their Whistler Server product. The product 
                          will now be called "Windows Server 2003".
         PJN / 30-01-2003 1. Added explicit support for detecting NT Service Pack 6a
         PJN / 08-02-2003 1. Added explicit support for detecting Windows XP Service Pack 1a
                          2. Added support to determine the HAL on NT Kernels.
         PJN / 12-02-2003 1. Fixed a compiler warning in GetNTServicePackFromRegistry which occurs when the code is compiled
                          with the Watcom compiler. Thanks to Christian Kaiser for reporting this.
         PJN / 08-03-2003 1. Updated a comment in COSVersion::GetProductSuiteDetailsFromRegistry re NT Embedded.
                          2. A comment from John A. Vorchak: On NTe (NT Embedded) and XPE (XP Embedded), all of the versions 
                          (of DTWinver) work just fine so long as the components to support them are included in the images, 
                          which itself is kind of a crap shoot.  I think that you would probably find that most images will 
                          not support the DOS or Win16 versions however most will support the Win32.  Many of the images that 
                          folks build either do not include the DOS subsystem and some of them do not include Explorer, so it 
                          really can't be said that all builds will support them however it is not difficult for a developer 
                          to understand which version would work for them as they understand their target systems better than 
                          anyone and at least one version would certainly work for almost all images. 
                          As far as Win2k (Server Appliance Kit), I haven't done enough testing with that platform, nor do I 
                          currently have any built images with the SAK to say positively or otherwise. More than likely you 
                          would find no problems with the SAK images since they typically follow W2k much more than NTe or 
                          XPE do.
                          Author: If you are writing for an embedded OS, then there is little use for DTWinver!!, since the
                          developer has very tight control over the runtime environment. Also if you do use DTWinver on an 
                          embedded version of Windows, you will probably compile in the dtwinver code rather than ship the 
                          sample binaries I include in the dtwinver download.
         PJN / 09-04-2004 1. Removed a number of unreferrenced variable warnings when you compile the code on on VS.NET 2003.
                          Thanks to Edward Livingston for reporting these issues.
                          2. Now includes support for Windows XP Media Center Edition. Please note that if you want to do
                          specific version checking of what version of Media Center you have installed then you should use
                          my CVersionInfo classes at http://www.naughter.com/versioninfo.html in conjunction with the following 
                          information which I culled from http://www.mvps.org/marksxp/MediaCenter/2004/version.php which
                          describes the various version numbers of ehshell.exe in \Windows\ehome to the corresponding versions
                          of XP Media Center Edition.
                         
                          Windows XP Media Center Edition: 2002    5.1.2600.1106 First released version of Windows Media Center  
                                                                   5.1.2600.1142 Highest released build of Media Center 2002 
                                                                                 (provided via Q815487) 
                          Windows XP Media Center Edition: 2004    5.1.2600.1217 Release build of Windows Media Center 2004 
                                                                                 (upgrade over previous MCE 2002 build).  
                                                                   5.1.2600.1321 December 2003 Hotfix for Media Center 2004 version 
                                                                                 (provided via Q830786) 
                                                                   5.1.2600.2096 Media Center Version included with Windows XP 
                                                                                 Service Pack 2 Release Candidate 1. This version can 
                                                                                 be installed over a current MCE 2002 or 2004 using 
                                                                                 the Windows XP Service Pack 2 installer. If you have 
                                                                                 any build between 1322 and 2095 assume this to be 
                                                                                 a beta version.  
                          3. dtwinver now returns the processor architecture via a call to GetSystemInfo or GetNativeSystemInfo. 
                          This is used to differentiate between 64 Bit Windows on Itanium and AMD64 processors.
                          4. Renamed the global preprocesor defines used by dtwinver to use more unique names
                          5. Added make files and binaries for AMD64 processors

Copyright (c) 1997 - 2004 by PJ Naughter.  (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 
*/


/////////////////////////////////  Includes  //////////////////////////////////
#include <windows.h> 
#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#else
#include <ctype.h>
#include <stdlib.h>
#include <shellapi.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "dtwinver.h"


/////////////////////////////////  Local function / variables /////////////////

//Taken from Platform SDK's WinNT.h file
#ifndef VER_NT_WORKSTATION
#define VER_NT_WORKSTATION              0x0000001
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_NT_DOMAIN_CONTROLLER
#define VER_NT_DOMAIN_CONTROLLER        0x0000002
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_NT_SERVER
#define VER_NT_SERVER                   0x0000003
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef PROCESSOR_ARCHITECTURE_SHX
#define PROCESSOR_ARCHITECTURE_SHX    4
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef PROCESSOR_ARCHITECTURE_ARM
#define PROCESSOR_ARCHITECTURE_ARM    5
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef PROCESSOR_ARCHITECTURE_IA64
#define PROCESSOR_ARCHITECTURE_IA64    6
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef PROCESSOR_ARCHITECTURE_ALPHA64
#define PROCESSOR_ARCHITECTURE_ALPHA64    7
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef PROCESSOR_ARCHITECTURE_MSIL
#define PROCESSOR_ARCHITECTURE_MSIL    8
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64    9
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64    10
#endif



//OS_VERSION_INFO::wSuiteMask can any of the following bit masks

//Taken from Platform SDK's WinNT.h file
#ifndef VER_SUITE_SMALLBUSINESS
#define VER_SUITE_SMALLBUSINESS             0x00000001
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_SUITE_ENTERPRISE
#define VER_SUITE_ENTERPRISE                0x00000002
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_SUITE_TERMINAL
#define VER_SUITE_TERMINAL                  0x00000010
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_SUITE_DATACENTER
#define VER_SUITE_DATACENTER                0x00000080
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_SUITE_PERSONAL
#define VER_SUITE_PERSONAL                  0x00000200
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_SUITE_BLADE
#define VER_SUITE_BLADE                     0x00000400
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_SUITE_EMBEDDEDNT
#define VER_SUITE_EMBEDDEDNT                0x00000040
#endif

//Taken from Platform SDK's WinNT.h file
#ifndef VER_SUITE_SINGLEUSERTS
#define VER_SUITE_SINGLEUSERTS              0x00000100
#endif


#if defined(_WIN32)

//Taken from Windows CE winbase.h file
#ifndef VER_PLATFORM_WIN32_CE
  #define VER_PLATFORM_WIN32_CE         3
#endif

#endif //defined(_WIN32) 

#ifdef _DOS
  WORD WinVer;
  BYTE bRunningWindows;
#endif //ifdef _DOS


////////////////////////////////// Implementation /////////////////////////////
COSVersion::COSVersion()
{
#if defined(_WINDOWS) && !defined(_WIN32) && !defined(_WIN64)
  //Initialize the values to sane defaults
  m_lpfnLoadLibraryEx32W = NULL;
  m_lpfnFreeLibrary32W = NULL;
  m_lpfnGetProcAddress32W = NULL;
  m_lpfnCallProcEx32W = NULL;
  m_hAdvApi32 = 0;    
  m_hKernel32 = 0;
  m_lpfnRegQueryValueExA= 0;
  m_lpfnGetVersionExA = 0;
  m_lpfnGetVersion = 0;
  m_lpfnGetSystemInfo = 0;
  m_lpfnGetNativeSystemInfo = 0;

  //Get Kernel dll handle
  HMODULE hKernel = GetModuleHandle("KERNEL");
  if (hKernel)
  {
    //Dynamically link to the functions we want
    m_lpfnLoadLibraryEx32W  = (lpfnLoadLibraryEx32W)  GetProcAddress(hKernel, "LoadLibraryEx32W");
    m_lpfnFreeLibrary32W    = (lpfnFreeLibrary32W)    GetProcAddress(hKernel, "FreeLibrary32W");
    m_lpfnGetProcAddress32W = (lpfnGetProcAddress32W) GetProcAddress(hKernel, "GetProcAddress32W");
    m_lpfnCallProcEx32W     = (lpfnCallProcEx32W)     GetProcAddress(hKernel, "_CallProcEx32W");
    if (m_lpfnLoadLibraryEx32W && m_lpfnFreeLibrary32W)
    {
      m_hAdvApi32 = m_lpfnLoadLibraryEx32W("ADVAPI32.DLL", NULL, 0);
      if (m_hAdvApi32)                                                   
        m_lpfnRegQueryValueExA = m_lpfnGetProcAddress32W(m_hAdvApi32, "RegQueryValueExA"); 
      m_hKernel32 = m_lpfnLoadLibraryEx32W("KERNEL32.DLL", NULL, 0);
      if (m_hKernel32)
      {                                                               
        m_lpfnGetVersionExA = m_lpfnGetProcAddress32W(m_hKernel32, "GetVersionExA");
        m_lpfnGetVersion = m_lpfnGetProcAddress32W(m_hKernel32, "GetVersion");
        m_lpfnGetSystemInfo = m_lpfnGetProcAddress32W(m_hKernel32, "GetSystemInfo");
        m_lpfnGetNativeSystemInfo = m_lpfnGetProcAddress32W(m_hKernel32, "GetNativeSystemInfo");
      }
    }
  }
#endif
}

COSVersion::~COSVersion()
{
#if defined(_WINDOWS) && !defined(_WIN32) && !defined(_WIN64)
  if (m_lpfnFreeLibrary32W)
  {           
    if (m_hAdvApi32)
      m_lpfnFreeLibrary32W(m_hAdvApi32);
    if (m_hKernel32)
      m_lpfnFreeLibrary32W(m_hKernel32);
  }
#endif
}

#if defined(_WINDOWS) && (!defined(_WIN32) && !defined(_WIN64))
BOOL COSVersion::WFWLoaded()
{
  const WORD WNNC_NET_MultiNet         = 0x8000;
  const WORD WNNC_SUBNET_WinWorkgroups = 0x0004;
  const WORD WNNC_NET_TYPE             = 0x0002;
  BOOL rVal;
   
  HINSTANCE hUserInst = LoadLibrary("USER.EXE");
  lpfnWNetGetCaps lpWNetGetCaps = (lpfnWNetGetCaps) GetProcAddress(hUserInst, "WNetGetCaps");
  if (lpWNetGetCaps != NULL)
  {
    // Get the network type
    WORD wNetType = lpWNetGetCaps(WNNC_NET_TYPE);
    if (wNetType & WNNC_NET_MultiNet)
    {
      // a multinet driver is installed
      if (LOBYTE(wNetType) & WNNC_SUBNET_WinWorkgroups) // It is WFW
        rVal = TRUE;
      else // It is not WFW
        rVal = FALSE;
    }
    else
     rVal = FALSE;
  }
  else
    rVal = FALSE;
   
  //Clean up the module instance
  if (hUserInst)
    FreeLibrary(hUserInst);
    
  return rVal;  
}

LONG COSVersion::RegQueryValueEx(HKEY32 hKey, LPSTR  lpszValueName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE  lpbData, LPDWORD lpcbData)
{                                             
  //Assume the worst
  DWORD dwResult = ERROR_CALL_NOT_IMPLEMENTED;

  if (m_lpfnRegQueryValueExA)
    dwResult = m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 6, 0x3e, m_lpfnRegQueryValueExA, hKey, lpszValueName, lpdwReserved, lpdwType, lpbData, lpcbData);

  return dwResult;
}                 

BOOL COSVersion::GetVersionEx(LPOSVERSIONINFO lpVersionInfo)
{
  //Assume the worst
  BOOL bSuccess = FALSE;

  if (m_lpfnGetVersionExA)
    bSuccess = (BOOL) m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 1, 1, m_lpfnGetVersionExA, lpVersionInfo, 0);

  return bSuccess;
}

void COSVersion::GetProcessorType(LPOS_VERSION_INFO lpVersionInformation)
{
  //Get the processor details
  SYSTEM_INFO EmulatedSI;
  EmulatedSI.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
  SYSTEM_INFO UnderlyingSI;
  UnderlyingSI.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

  if (m_lpfnGetNativeSystemInfo)
  {
    m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 1, 1, m_lpfnGetNativeSystemInfo, &UnderlyingSI, 0);
  
    if (m_lpfnGetSystemInfo)
      m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 1, 1, m_lpfnGetSystemInfo, &EmulatedSI, 0);
  }
  else
  {
    if (m_lpfnGetSystemInfo)
      m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 1, 1, m_lpfnGetSystemInfo, &EmulatedSI, 0);

    memcpy(&EmulatedSI, &UnderlyingSI, sizeof(SYSTEM_INFO));
  }

  //Map from the SYTEM_INFO wProcessorArchitecture defines to our equivalents
  switch (UnderlyingSI.wProcessorArchitecture)
  {
    case PROCESSOR_ARCHITECTURE_INTEL: //deliberate fallthrough
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
    {
      lpVersionInformation->UnderlyingProcessorType = INTEL_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_MSIL:
    {
      lpVersionInformation->UnderlyingProcessorType = MSIL_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_MIPS:
    {
      lpVersionInformation->UnderlyingProcessorType = MIPS_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ARM:
    {
      lpVersionInformation->UnderlyingProcessorType = ARM_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_SHX:
    {
      lpVersionInformation->UnderlyingProcessorType = SHX_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ALPHA:
    {
      lpVersionInformation->UnderlyingProcessorType = ALPHA_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ALPHA64:
    {
      lpVersionInformation->UnderlyingProcessorType = ALPHA64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_PPC:
    {
      lpVersionInformation->UnderlyingProcessorType = PPC_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_IA64:
    {
      lpVersionInformation->UnderlyingProcessorType = IA64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_AMD64:
    {
      lpVersionInformation->UnderlyingProcessorType = AMD64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_UNKNOWN: //Deliberate fallthrough
    default:
    {
      lpVersionInformation->UnderlyingProcessorType = UNKNOWN_PROCESSOR;
      break;
    }
  }
  switch (EmulatedSI.wProcessorArchitecture)
  {
    case PROCESSOR_ARCHITECTURE_INTEL: //deliberate fallthrough
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
    {
      lpVersionInformation->EmulatedProcessorType = INTEL_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_MSIL:
    {
      lpVersionInformation->EmulatedProcessorType = MSIL_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_MIPS:
    {
      lpVersionInformation->EmulatedProcessorType = MIPS_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ARM:
    {
      lpVersionInformation->EmulatedProcessorType = ARM_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_SHX:
    {
      lpVersionInformation->EmulatedProcessorType = SHX_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ALPHA:
    {
      lpVersionInformation->EmulatedProcessorType = ALPHA_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ALPHA64:
    {
      lpVersionInformation->EmulatedProcessorType = ALPHA64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_PPC:
    {
      lpVersionInformation->EmulatedProcessorType = PPC_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_IA64:
    {
      lpVersionInformation->EmulatedProcessorType = IA64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_AMD64:
    {
      lpVersionInformation->EmulatedProcessorType = AMD64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_UNKNOWN: //Deliberate fallthrough
    default:
    {
      lpVersionInformation->EmulatedProcessorType = UNKNOWN_PROCESSOR;
      break;
    }
  }
}

DWORD COSVersion::GetVersion()
{
  //Assume the worst
  DWORD dwVersion = 0;

  if (m_lpfnGetVersion)
    dwVersion = (BOOL) m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, m_lpfnGetVersion, 0);

  return dwVersion;
}
#endif //defined(_WINDOWS) && !defined(_WIN32)

BOOL COSVersion::GetVersion(LPOS_VERSION_INFO lpVersionInformation)
{
  //Zero out everything in the structure
  memset(lpVersionInformation, 0, sizeof(lpVersionInformation));
                
  #ifdef UNDER_CE
    OSVERSIONINFO osvi;
    memset(&osvi, 0, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osvi))
      return FALSE;

    //Basic OS info
    lpVersionInformation->dwUnderlyingMajorVersion = osvi.dwMajorVersion; 
    lpVersionInformation->dwUnderlyingMinorVersion = osvi.dwMinorVersion; 
    lpVersionInformation->dwUnderlyingBuildNumber = LOWORD(osvi.dwBuildNumber); //ignore HIWORD
    lpVersionInformation->UnderlyingPlatform = WindowsCE;
    _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, osvi.szCSDVersion);

    //OEM Info
    _tcscpy(lpVersionInformation->szOEMInfo, _T(""));
    SystemParametersInfo(SPI_GETOEMINFO, 256, lpVersionInformation->szOEMInfo, 0);

    //Platform Type
    _tcscpy(lpVersionInformation->szPlatformType, _T(""));
    SystemParametersInfo(SPI_GETPLATFORMTYPE, 256, lpVersionInformation->szPlatformType, 0);

    //Always set the OSType to Workstation on CE. The variable itself does not make 
    //much sense on CE, but we do not conditionally compile it out on CE as then we
    //would have to put in loadsa ifdefs UNDER_CE in the COSVersion::Is... methods
    lpVersionInformation->OSType = Workstation;

  #elif defined(_WIN32) || defined(_WIN64)
    //determine dynamically if GetVersionEx is available, if 
    //not then drop back to using GetVersion

    // Get Kernel handle
    HMODULE hKernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
    if (hKernel32 == NULL)
      return FALSE;

    #ifdef _UNICODE
      lpfnGetVersionEx lpGetVersionEx = (lpfnGetVersionEx) GetProcAddress(hKernel32, "GetVersionExW");
    #else
      lpfnGetVersionEx lpGetVersionEx = (lpfnGetVersionEx) GetProcAddress(hKernel32, "GetVersionExA");
    #endif

    if (lpGetVersionEx)
    {
      OSVERSIONINFOEX osviex;
      memset(&osviex, 0, sizeof(OSVERSIONINFOEX));
      osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

      OSVERSIONINFO osvi;
      memset(&osvi, 0, sizeof(OSVERSIONINFO));
      osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

      BOOL bGotOSviEx = lpGetVersionEx((LPOSVERSIONINFO) &osviex);
      if (!bGotOSviEx)
      {
        if (!lpGetVersionEx(&osvi))
          return FALSE;
      }

      if (bGotOSviEx)
      {
        lpVersionInformation->dwEmulatedMajorVersion = osviex.dwMajorVersion;
        lpVersionInformation->dwEmulatedMinorVersion = osviex.dwMinorVersion;
        lpVersionInformation->dwEmulatedBuildNumber = LOWORD(osviex.dwBuildNumber);
        _tcscpy(lpVersionInformation->szEmulatedCSDVersion, osviex.szCSDVersion);
        lpVersionInformation->wEmulatedServicePackMajor = osviex.wServicePackMajor;
        lpVersionInformation->wEmulatedServicePackMinor = osviex.wServicePackMinor;
        
        //Explicitly map the Win32 wSuiteMask to our own values
        if (osviex.wSuiteMask & VER_SUITE_SMALLBUSINESS)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SMALLBUSINESS;
        if (osviex.wSuiteMask & VER_SUITE_ENTERPRISE)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
        if (osviex.wSuiteMask & VER_SUITE_TERMINAL)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_TERMINAL;
        if (osviex.wSuiteMask & VER_SUITE_DATACENTER)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_DATACENTER;
        if (osviex.wSuiteMask & VER_SUITE_PERSONAL)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_PERSONAL;
        if (osviex.wSuiteMask & VER_SUITE_BLADE)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_WEBEDITION;
        if (osviex.wSuiteMask & VER_SUITE_EMBEDDEDNT)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDEDNT;
        if (osviex.wSuiteMask & VER_SUITE_SINGLEUSERTS)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL;

        //Explicitely map the Win32 wProductType to our own values
        switch (osviex.wProductType)
        {
          case VER_NT_WORKSTATION:
          {
            lpVersionInformation->OSType = Workstation;
            break;
          }
          case VER_NT_DOMAIN_CONTROLLER:
          {
            lpVersionInformation->OSType = DomainController;
            break;
          }
          case VER_NT_SERVER:
          {
            lpVersionInformation->OSType = Server;
            break;
          }
          default:
          {
            break;
          }
        }

        //Determine the Domain Controller Type
        GetNTOSTypeFromRegistry(lpVersionInformation, TRUE);

        //Get the media center details
        GetMediaCenterDetailsFromRegistry(lpVersionInformation);

        //Explicitly map the win32 dwPlatformId to our own values 
        switch (osviex.dwPlatformId)
        {
          case VER_PLATFORM_WIN32_WINDOWS:
          {
            lpVersionInformation->EmulatedPlatform = Windows9x;
            break;
          }
          case VER_PLATFORM_WIN32_NT:
          {
            lpVersionInformation->EmulatedPlatform = WindowsNT;
            break;
          }
          case VER_PLATFORM_WIN32_CE:
          {
            lpVersionInformation->EmulatedPlatform = WindowsCE;
            break;
          }
          default:
          {
            break;
          }
        }

        lpVersionInformation->dwUnderlyingMajorVersion = lpVersionInformation->dwEmulatedMajorVersion; 
        lpVersionInformation->dwUnderlyingMinorVersion = lpVersionInformation->dwEmulatedMinorVersion; 
        lpVersionInformation->dwUnderlyingBuildNumber = lpVersionInformation->dwEmulatedBuildNumber;
        lpVersionInformation->UnderlyingPlatform = lpVersionInformation->EmulatedPlatform;
        _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, lpVersionInformation->szEmulatedCSDVersion);
        lpVersionInformation->wUnderlyingServicePackMajor = lpVersionInformation->wEmulatedServicePackMajor;
        lpVersionInformation->wUnderlyingServicePackMinor = lpVersionInformation->wEmulatedServicePackMinor;

      #ifndef UNDER_CE
        //Determine if it is NT SP6a Vs SP6
        GetNTSP6aDetailsFromRegistry(lpVersionInformation, TRUE);

        //Determine if it is XP SP1a Vs SP1
        GetXPSP1aDetailsFromRegistry(lpVersionInformation, TRUE);

        //Determine the HAL details
        GetNTHALDetailsFromRegistry(lpVersionInformation);
	    #endif
      }
      else
      {
        lpVersionInformation->dwEmulatedMajorVersion = osvi.dwMajorVersion; 
        lpVersionInformation->dwEmulatedMinorVersion = osvi.dwMinorVersion; 
        lpVersionInformation->dwEmulatedBuildNumber = LOWORD(osvi.dwBuildNumber); //ignore HIWORD
        _tcscpy(lpVersionInformation->szEmulatedCSDVersion, osvi.szCSDVersion);
    
        //Explicitely map the win32 dwPlatformId to our own values 
        //Also work out the various service packs which are installed
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        {
          lpVersionInformation->EmulatedPlatform = Windows9x;
          lpVersionInformation->OSType = Workstation;

          //Deterine the Win9x Service pack installed
          if (IsWindows95SP1(lpVersionInformation))
          {
            lpVersionInformation->wEmulatedServicePackMajor = 1;
            lpVersionInformation->wUnderlyingServicePackMajor = 1;
          }
          else if (IsWindows95OSR2(lpVersionInformation))
          {
            lpVersionInformation->wEmulatedServicePackMajor = 2;
            lpVersionInformation->wUnderlyingServicePackMajor = 2;
          }
          else if (IsWindows98SP1(lpVersionInformation))
          {
            lpVersionInformation->wEmulatedServicePackMajor = 1;
            lpVersionInformation->wUnderlyingServicePackMajor = 1;
          }
        }
        else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
          lpVersionInformation->EmulatedPlatform = WindowsNT;

          //Determine the NT Service pack
          lpVersionInformation->wEmulatedServicePackMajor = GetNTServicePackFromCSDString(osvi.szCSDVersion);
          lpVersionInformation->wUnderlyingServicePackMajor = lpVersionInformation->wEmulatedServicePackMajor;

          //Determine the OS Type
          GetNTOSTypeFromRegistry(lpVersionInformation, FALSE);

          //Get the Product Suites installed
          GetProductSuiteDetailsFromRegistry(lpVersionInformation);
          GetTerminalServicesRemoteAdminModeDetailsFromRegistry(lpVersionInformation);
        }
        else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_CE)
        {
          lpVersionInformation->EmulatedPlatform = WindowsCE;

          //Always set the OSType to Workstation on CE. The variable itself does not make 
          //much sense on CE, but we do not conditionally compile it out on CE as then we
          //would have to put in loadsa ifdefs UNDER_CE in the COSVersion::Is... methods
          lpVersionInformation->OSType = Workstation;
        }

        if (osvi.dwPlatformId == VER_PLATFORM_WIN32s)  //32 bit app running on Windows 3.x
        {
          lpVersionInformation->EmulatedPlatform = Windows9x;

          lpVersionInformation->dwUnderlyingMajorVersion = 3; 
          lpVersionInformation->dwUnderlyingMinorVersion = 10; 
          lpVersionInformation->dwUnderlyingBuildNumber = 0;
          lpVersionInformation->UnderlyingPlatform = Windows3x;
          _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, _T(""));
        }
        else
        {
          lpVersionInformation->dwUnderlyingMajorVersion = lpVersionInformation->dwEmulatedMajorVersion; 
          lpVersionInformation->dwUnderlyingMinorVersion = lpVersionInformation->dwEmulatedMinorVersion; 
          lpVersionInformation->dwUnderlyingBuildNumber = lpVersionInformation->dwEmulatedBuildNumber;
          lpVersionInformation->UnderlyingPlatform = lpVersionInformation->EmulatedPlatform;
          _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, lpVersionInformation->szEmulatedCSDVersion);
        }

        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
          //Determine if it is NT SP6a Vs SP6
          GetNTSP6aDetailsFromRegistry(lpVersionInformation, TRUE);

          //Determine if it is XP SP1a Vs SP1
          GetXPSP1aDetailsFromRegistry(lpVersionInformation, TRUE);

          //Determine the HAL details
          GetNTHALDetailsFromRegistry(lpVersionInformation);
        }
      }
    }
    else
    {
      //Since GetVersionEx is not available we need to fall back on plain
      //old GetVersion. Because GetVersionEx is available on all but v3.1 of NT
      //we can fill in some of the parameters ourselves.
      DWORD dwVersion = ::GetVersion();

      lpVersionInformation->dwEmulatedMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
      lpVersionInformation->dwEmulatedMinorVersion =  (DWORD)(HIBYTE(LOWORD(dwVersion)));
      lpVersionInformation->dwEmulatedBuildNumber = 0;
      lpVersionInformation->EmulatedPlatform   = WindowsNT;   
      lpVersionInformation->dwUnderlyingMajorVersion = lpVersionInformation->dwEmulatedMajorVersion;
      lpVersionInformation->dwUnderlyingMinorVersion = lpVersionInformation->dwEmulatedMinorVersion;
      lpVersionInformation->dwUnderlyingBuildNumber  = lpVersionInformation->dwEmulatedBuildNumber;
      lpVersionInformation->UnderlyingPlatform   = lpVersionInformation->EmulatedPlatform;   
      _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, lpVersionInformation->szEmulatedCSDVersion);

      //Need to determine the NT Service pack by querying the registry directory.
      lpVersionInformation->wEmulatedServicePackMajor = GetNTServicePackFromRegistry();
      lpVersionInformation->wUnderlyingServicePackMajor = lpVersionInformation->wEmulatedServicePackMajor;
    }

  #ifndef UNDER_CE
    //Get the processor details
    if (hKernel32)
    {
      SYSTEM_INFO EmulatedSI;
      SYSTEM_INFO UnderlyingSI;

      lpfnGetNativeSystemInfo pGetNativeSystemInfo = (lpfnGetNativeSystemInfo) GetProcAddress(hKernel32, "GetNativeSystemInfo"); 
      if (pGetNativeSystemInfo)
      {
        pGetNativeSystemInfo(&UnderlyingSI);
        GetSystemInfo(&EmulatedSI);
      }
      else
      {
        GetSystemInfo(&UnderlyingSI);
        CopyMemory(&EmulatedSI, &UnderlyingSI, sizeof(SYSTEM_INFO));
      }

      //Map from the SYTEM_INFO wProcessorArchitecture defines to our equivalents
      switch (UnderlyingSI.wProcessorArchitecture)
      {
        case PROCESSOR_ARCHITECTURE_INTEL: //deliberate fallthrough
        case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
        {
          lpVersionInformation->UnderlyingProcessorType = INTEL_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_MSIL:
        {
          lpVersionInformation->UnderlyingProcessorType = MSIL_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_MIPS:
        {
          lpVersionInformation->UnderlyingProcessorType = MIPS_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_ARM:
        {
          lpVersionInformation->UnderlyingProcessorType = ARM_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_SHX:
        {
          lpVersionInformation->UnderlyingProcessorType = SHX_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_ALPHA:
        {
          lpVersionInformation->UnderlyingProcessorType = ALPHA_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_ALPHA64:
        {
          lpVersionInformation->UnderlyingProcessorType = ALPHA64_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_PPC:
        {
          lpVersionInformation->UnderlyingProcessorType = PPC_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_IA64:
        {
          lpVersionInformation->UnderlyingProcessorType = IA64_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_AMD64:
        {
          lpVersionInformation->UnderlyingProcessorType = AMD64_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_UNKNOWN: //Deliberate fallthrough
        default:
        {
          lpVersionInformation->UnderlyingProcessorType = UNKNOWN_PROCESSOR;
          break;
        }
      }

      switch (EmulatedSI.wProcessorArchitecture)
      {
        case PROCESSOR_ARCHITECTURE_INTEL: //deliberate fallthrough
        case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
        {
          lpVersionInformation->EmulatedProcessorType = INTEL_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_MSIL:
        {
          lpVersionInformation->EmulatedProcessorType = MSIL_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_MIPS:
        {
          lpVersionInformation->EmulatedProcessorType = MIPS_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_ARM:
        {
          lpVersionInformation->EmulatedProcessorType = ARM_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_SHX:
        {
          lpVersionInformation->EmulatedProcessorType = SHX_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_ALPHA:
        {
          lpVersionInformation->EmulatedProcessorType = ALPHA_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_ALPHA64:
        {
          lpVersionInformation->EmulatedProcessorType = ALPHA64_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_PPC:
        {
          lpVersionInformation->EmulatedProcessorType = PPC_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_IA64:
        {
          lpVersionInformation->EmulatedProcessorType = IA64_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_AMD64:
        {
          lpVersionInformation->EmulatedProcessorType = AMD64_PROCESSOR;
          break;
        }
        case PROCESSOR_ARCHITECTURE_UNKNOWN: //Deliberate fallthrough
        default:
        {
          lpVersionInformation->EmulatedProcessorType = UNKNOWN_PROCESSOR;
          break;
        }
      }
    }
  #endif
  
  #else //We must be runing on an emulated or real version of Win16 or DOS
    lpVersionInformation->EmulatedProcessorType = INTEL_PROCESSOR; //We can only be running Intel(x86) code from Win16 or DOS

    #ifdef _WINDOWS //Running on some version of Windows                   
      DWORD dwVersion = GetVersion();
      // GetVersion does not differentiate between Windows 3.1 and Windows 3.11
      
      lpVersionInformation->dwEmulatedMajorVersion = LOBYTE(LOWORD(dwVersion)); 
      lpVersionInformation->dwEmulatedMinorVersion = HIBYTE(LOWORD(dwVersion));
      lpVersionInformation->dwEmulatedBuildNumber  = 0; //no build number with Win3.1x
      lpVersionInformation->EmulatedPlatform       = Windows3x;
      
      //Call to get the underlying OS here through 16 -> 32 bit Generic Thunk
      BOOL bFoundUnderlyingOS = FALSE;
      OSVERSIONINFO osvi;                      
      memset(&osvi, 0, sizeof(OSVERSIONINFO));
      osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
      if (GetVersionEx(&osvi))
      {
        lpVersionInformation->dwUnderlyingMajorVersion = osvi.dwMajorVersion; 
        lpVersionInformation->dwUnderlyingMinorVersion = osvi.dwMinorVersion; 
        lpVersionInformation->dwUnderlyingBuildNumber = LOWORD(osvi.dwBuildNumber); //ignore HIWORD
        _fstrcpy(lpVersionInformation->szUnderlyingCSDVersion, osvi.szCSDVersion);
       
        //Explicitely map the win32 dwPlatformId to our own values
        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        {
          lpVersionInformation->UnderlyingPlatform = Windows9x;

          //Deterine the Win9x Service pack installed
          if (IsWindows95SP1(lpVersionInformation))
            lpVersionInformation->wUnderlyingServicePackMajor = 1;
          else if (IsWindows95OSR2(lpVersionInformation))
            lpVersionInformation->wUnderlyingServicePackMajor = 2;
          else if (IsWindows98SP1(lpVersionInformation))
            lpVersionInformation->wUnderlyingServicePackMajor = 1;
        }
        else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
          lpVersionInformation->UnderlyingPlatform = WindowsNT;

          //Determine the NT Service pack
          lpVersionInformation->wEmulatedServicePackMajor = 0; //Win16 does not have a concept of a service pack
          lpVersionInformation->wUnderlyingServicePackMajor = GetNTServicePackFromCSDString(osvi.szCSDVersion);

          //Determine the OS Type
          GetNTOSTypeFromRegistry(lpVersionInformation, FALSE);

		#ifndef UNDER_CE
          //Determine if it is NT SP6a Vs SP6
          GetNTSP6aDetailsFromRegistry(lpVersionInformation, FALSE);

          //Determine if it is XP SP1a Vs SP1
          GetXPSP1aDetailsFromRegistry(lpVersionInformation, FALSE);

          //Determine the HAL details
          GetNTHALDetailsFromRegistry(lpVersionInformation);
		#endif

          //Get the Product Suites installed
          GetProductSuiteDetailsFromRegistry(lpVersionInformation);
          GetTerminalServicesRemoteAdminModeDetailsFromRegistry(lpVersionInformation);
          GetMediaCenterDetailsFromRegistry(lpVersionInformation);
        }
        else
          return FALSE;
       
        bFoundUnderlyingOS = TRUE;
      }
      else
      {
        //We failed to get GetVersionEx so try to GetVersion instead. We known that we must be on
        //Windows NT 3.5 or earlier since anything released later by MS had this function.
        DWORD dwVersion = GetVersion();
        if (dwVersion)
        {              
          lpVersionInformation->dwUnderlyingMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
          lpVersionInformation->dwUnderlyingMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
          lpVersionInformation->dwUnderlyingBuildNumber  = 0;
          lpVersionInformation->UnderlyingPlatform   = WindowsNT; 
          _fstrcpy(lpVersionInformation->szUnderlyingCSDVersion, "");
   
          bFoundUnderlyingOS = TRUE;
        }
      }
                                             
      //Get the processor details                                       
	  GetProcessorType(lpVersionInformation);

      if (!bFoundUnderlyingOS)
      {
        //must be running on a real version of 16 bit Windows whose underlying OS is DOS
        lpVersionInformation->dwUnderlyingMajorVersion = HIBYTE(HIWORD(dwVersion)); 
        lpVersionInformation->dwUnderlyingMinorVersion = LOBYTE(HIWORD(dwVersion)); 
        lpVersionInformation->dwUnderlyingBuildNumber = 0; 
        lpVersionInformation->UnderlyingPlatform = Dos;
        _fstrcpy(lpVersionInformation->szUnderlyingCSDVersion, "");
      }
    #else //Must be some version of real or emulated DOS
      //Retreive the current version of emulated DOS
      BYTE DosMinor;
      BYTE DosMajor;
      _asm
      {
        mov ax, 3306h
        int 21h
        mov byte ptr [DosMajor], bl
        mov byte ptr [DosMinor], bh
      }
      lpVersionInformation->EmulatedPlatform = Dos;
      lpVersionInformation->dwEmulatedMajorVersion = (DWORD) DosMajor; 
      lpVersionInformation->dwEmulatedMinorVersion = (DWORD) DosMinor;                
      lpVersionInformation->dwEmulatedBuildNumber = 0; //no build number with DOS
      
      //See can we get the underlying OS info by calling WriteVer
      if (!GetInfoBySpawingWriteVer(lpVersionInformation))
      {
        //We can detect if NT is running as it reports DOS v5.5
        if ((lpVersionInformation->dwEmulatedMajorVersion == 5) &&
            (lpVersionInformation->dwEmulatedMinorVersion == 50))    //NT reports DOS v5.5
        {
          _fstrcpy(lpVersionInformation->szUnderlyingCSDVersion, "");    
          //could not find method of determing version of NT from DOS,
          //so assume 3.50
          lpVersionInformation->dwUnderlyingMajorVersion = 3; 
          lpVersionInformation->dwUnderlyingMinorVersion = 50; 
          lpVersionInformation->dwUnderlyingBuildNumber = 0;  //cannot get access to build number from DOS
          lpVersionInformation->UnderlyingPlatform = WindowsNT;
        }            
        else
        {
          //Get the underlying OS here via the int 2FH interface of Windows
          GetWinInfo();
          if (bRunningWindows)
          { 
            if (lpVersionInformation->dwEmulatedMajorVersion >= 7)  //Windows 9x marks itself as DOS 7 or DOS 8
              lpVersionInformation->UnderlyingPlatform = Windows9x;
            else                                                              
            {
              //Could not find method of differentiating between WFW & Win3.1 under DOS,
              //so assume Win3.1                                     
              lpVersionInformation->UnderlyingPlatform = Windows3x;      
            }  
            _fstrcpy(lpVersionInformation->szUnderlyingCSDVersion, "");
            lpVersionInformation->dwUnderlyingMajorVersion = (WinVer & 0xFF00) >> 8; 
            lpVersionInformation->dwUnderlyingMinorVersion = WinVer & 0x00FF; 
  
            if (lpVersionInformation->dwEmulatedMajorVersion >= 8)  //Windows Me reports itself as DOS v8.0
              lpVersionInformation->dwUnderlyingBuildNumber = 3000; //This is the build number for Windows ME.
            else
            {
              if (lpVersionInformation->dwEmulatedMinorVersion == 0)
                lpVersionInformation->dwUnderlyingBuildNumber = 950; //Windows 95 Gold reports DOS v7.0                      
              else if (lpVersionInformation->dwUnderlyingMinorVersion > 0 && 
                       lpVersionInformation->dwUnderlyingMinorVersion < 3) 
              {                                                            
                //Testing for 95 SP1 has not been done, so the above check
                //may or may not work
                lpVersionInformation->dwUnderlyingBuildNumber = 1080; 
              }
              else if (lpVersionInformation->dwUnderlyingMinorVersion == 3)
                lpVersionInformation->dwUnderlyingBuildNumber = 1212; //Windows 95 OSR2 reports DOS 7.03 from 16 bit code
              else
                lpVersionInformation->dwUnderlyingBuildNumber = 1998; //Windows 98 or SE. There is no way to differentiate from real mode
                                                                      //between the two of them
            }
          }
          else //must be on a real version of DOS
          {                               
            lpVersionInformation->dwUnderlyingMajorVersion = (DWORD) DosMajor; 
            lpVersionInformation->dwUnderlyingMinorVersion = (DWORD) DosMinor;                
            lpVersionInformation->dwUnderlyingBuildNumber = 0; //no build number with DOS
            lpVersionInformation->UnderlyingPlatform = Dos;
            _fstrcpy(lpVersionInformation->szUnderlyingCSDVersion, "");
          }
        }   
      }
    #endif  
  #endif

  return TRUE;
}

#ifndef _DOS
void COSVersion::GetNTSP6aDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bUpdateEmulatedAlso)
{
#ifndef UNDER_CE
  if ((lpVersionInformation->dwUnderlyingMajorVersion == 4) && (lpVersionInformation->wUnderlyingServicePackMajor == 6))
  {
    //Test for SP6 versus SP6a.
    HKEY hKey;
#if (defined(_WIN32) || defined(_WIN64))
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
#else                                  
	if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"), &hKey) == ERROR_SUCCESS)
#endif    
    {
      lpVersionInformation->wUnderlyingServicePackMinor = 1;         
      if (bUpdateEmulatedAlso)
        lpVersionInformation->wEmulatedServicePackMinor = 1;
    }

    RegCloseKey(hKey);
  }
#endif
}
#endif

#ifndef _DOS
void COSVersion::GetXPSP1aDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bUpdateEmulatedAlso)
{
#ifndef UNDER_CE
  if ((lpVersionInformation->dwUnderlyingMajorVersion == 5) && (lpVersionInformation->dwUnderlyingMinorVersion != 0) && (lpVersionInformation->wUnderlyingServicePackMajor == 1))
  {
    //Test for SP1a versus SP1.
    HKEY hKey;
#if (defined(_WIN32) || defined(_WIN64))
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
#else                                  
	if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), &hKey) == ERROR_SUCCESS)
#endif    
    {
      TCHAR sTemp[1024];
      DWORD dwBufLen = 1024 * sizeof(TCHAR);

  #if (defined(_WIN32) || defined(_WIN64))
      if (::RegQueryValueEx(hKey, _T("SubVersionNumber"), NULL, NULL, (LPBYTE) sTemp, &dwBufLen) == ERROR_SUCCESS)
  #else
      if (RegQueryValueEx(hKey, _T("SubVersionNumber"), NULL, NULL, (LPBYTE) sTemp, &dwBufLen) == ERROR_SUCCESS)
  #endif
      {
        if (_tcsicmp(sTemp, _T("a")) == 0)
        {
          lpVersionInformation->wUnderlyingServicePackMinor = 1;         
          if (bUpdateEmulatedAlso)
            lpVersionInformation->wEmulatedServicePackMinor = 1;
        }
      }
    }

    RegCloseKey(hKey);
  }
#endif
}
#endif

#ifndef _DOS
void COSVersion::GetNTHALDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation)
{
#ifndef UNDER_CE
  HKEY hKey;
#if (defined(_WIN32) || defined(_WIN64))
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
#else                                  
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), &hKey) == ERROR_SUCCESS)
#endif    
  {
    TCHAR sTemp[1024];
    DWORD dwBufLen = 1024 * sizeof(TCHAR);

#if (defined(_WIN32) || defined(_WIN64))
    if (::RegQueryValueEx(hKey, _T("CurrentType"), NULL, NULL, (LPBYTE) sTemp, &dwBufLen) == ERROR_SUCCESS)
#else
    if (RegQueryValueEx(hKey, _T("CurrentType"), NULL, NULL, (LPBYTE) sTemp, &dwBufLen) == ERROR_SUCCESS)
#endif
    {
      if (_tcsicmp(sTemp, _T("Uniprocessor Free")) == 0)
        lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_UNIPROCESSOR_FREE;         
      else if (_tcsicmp(sTemp, _T("Uniprocessor Checked")) == 0)
        lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_UNIPROCESSOR_CHECKED;         
      else if (_tcsicmp(sTemp, _T("Multiprocessor Free")) == 0)
        lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_MULTIPROCESSOR_FREE;         
      else if (_tcsicmp(sTemp, _T("Multiprocessor Checked")) == 0)
        lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_MULTIPROCESSOR_CHECKED;         
    }
  }

  RegCloseKey(hKey);
#endif
}
#endif
      
#ifndef _DOS
void COSVersion::GetNTOSTypeFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bOnlyUpdateDCDetails)
{
  //Open and the product options key
  HKEY hKey;
#if (defined(_WIN32) || defined(_WIN64))
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
#else                                                                                                                               
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), &hKey) == ERROR_SUCCESS)
#endif
  {
    TCHAR sTemp[1024];
    DWORD dwBufLen = 1024 * sizeof(TCHAR);

#if (defined(_WIN32) || defined(_WIN64))
    if (::RegQueryValueEx(hKey, _T("ProductType"), NULL, NULL, (LPBYTE) sTemp, &dwBufLen) == ERROR_SUCCESS)
#else
    if (RegQueryValueEx(hKey, _T("ProductType"), NULL, NULL, (LPBYTE) sTemp, &dwBufLen) == ERROR_SUCCESS)
#endif
    {
      if (bOnlyUpdateDCDetails)
      {
        if (_tcsicmp(sTemp, _T("LANMANNT")) == 0)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_PRIMARY_DOMAIN_CONTROLLER;
        else if (_tcsicmp(sTemp, _T("LANSECNT")) == 0)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_BACKUP_DOMAIN_CONTROLLER;
      }
      else
      {
        if (_tcsicmp(sTemp, _T("LANMANNT")) == 0)
        {
          lpVersionInformation->OSType = DomainController;
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_PRIMARY_DOMAIN_CONTROLLER;
        }
        else if (_tcsicmp(sTemp, _T("SERVERNT")) == 0)
        {
          lpVersionInformation->OSType = Server;
        }
        else if (_tcsicmp(sTemp, _T("LANSECNT")) == 0)
        {
          lpVersionInformation->OSType = DomainController;
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_BACKUP_DOMAIN_CONTROLLER;
        }
        else if (_tcsicmp(sTemp, _T("WinNT")) == 0)
        {
          lpVersionInformation->OSType = Workstation;
        }
      }
    }

    RegCloseKey(hKey);
  }
}
#endif //#ifndef _DOS
                                   
#ifdef _DOS             
BOOL COSVersion::GetInfoBySpawingWriteVer(COSVersion::LPOS_VERSION_INFO lpVersionInformation)
{
  //Assume the worst
  BOOL bSuccess = FALSE;

  //Form the command line we need  
  char pszCommandLine[256];      
  char* pszTempFilename = tmpnam(NULL);
  sprintf(pszCommandLine, "WriteVer.exe %s", pszTempFilename);

  //Try to spawn out writever utilty
  if (system(pszCommandLine) != -1)
  {           
    //Open the file we need                 
    FILE* pOSFile = fopen(pszTempFilename, "r");
    if (pOSFile == NULL)
      return bSuccess;
        
    //Read in the OS version info from file
    char pszLine[1024];
    size_t nRead = fread(pszLine, 1, 1024, pOSFile);
    if (nRead)
    {
      pszLine[nRead] = '\0'; //Don't forget to NULL terminate

      //Parse the input string                               
      char* pszSeparators = "\t";                               
      char* pszToken = strtok(pszLine, pszSeparators);
      if (pszToken)
      {
        lpVersionInformation->dwUnderlyingMajorVersion = atoi(pszToken);      
        pszToken = strtok(NULL, pszSeparators);         
        if (pszToken)
        {
          lpVersionInformation->dwUnderlyingMinorVersion = atoi(pszToken);      
          pszToken = strtok(NULL, pszSeparators);         
          if (pszToken)
          {
            lpVersionInformation->dwUnderlyingBuildNumber = atoi(pszToken);       
            pszToken = strtok(NULL, pszSeparators);         
            if (pszToken)
            {
              lpVersionInformation->UnderlyingPlatform = (COSVersion::OS_PLATFORM) atoi(pszToken);       
              pszToken = strtok(NULL, pszSeparators);         
              if (pszToken)                                       
              {                                                   
                lpVersionInformation->wUnderlyingServicePackMajor = (WORD) atoi(pszToken);       
                pszToken = strtok(NULL, pszSeparators);             
                if (pszToken)
                {
                  lpVersionInformation->wUnderlyingServicePackMinor = (WORD) atoi(pszToken);       
                  pszToken = strtok(NULL, pszSeparators);             
                  if (pszToken)
                  {
                    lpVersionInformation->dwSuiteMask = (DWORD) atoi(pszToken);       
                    pszToken = strtok(NULL, pszSeparators);
                    if (pszToken)
                    {
                      lpVersionInformation->OSType = (COSVersion::OS_TYPE) atoi(pszToken);       

                      pszToken = strtok(NULL, pszSeparators);             
                      if (pszToken)
                      {
                        lpVersionInformation->UnderlyingProcessorType = (COSVersion::PROCESSOR_TYPE) atoi(pszToken);       

                        //CSDVersion string is optional so consider it successful if we got as far as here
                        bSuccess = TRUE;

                        pszToken = strtok(NULL, pszSeparators);
                        if (pszToken)
                          strcpy(lpVersionInformation->szUnderlyingCSDVersion, pszToken);                                    
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
   
    //Don't forget to close the file
    fclose(pOSFile);

    //and remove it now that we are finished with it
    remove(pszTempFilename);
  }
  else
  {
    printf("writever command failed, Errno:%d\n", errno);
  }     

  return bSuccess;
}

void COSVersion::GetWinInfo()
{ 
  BYTE MajorVer;
  BYTE MinorVer;

  //use some inline assembly to determine if Windows if
  //running and what version is active
  _asm
  {
  ; check for Windows 3.1
    mov     ax,160ah                ; WIN31CHECK
    int     2fh                     ; check if running under Win 3.1.
    or      ax,ax
    jz      RunningUnderWin31       ; can check if running in standard
                                    ; or enhanced mode
   
  ; check for Windows 3.0 enhanced mode
    mov     ax,1600h                ; WIN386CHECK
    int     2fh
    test    al,7fh
    jnz     RunningUnderWin30Enh    ; enhanced mode
   
  ; check for 3.0 WINOLDAP
    mov     ax,4680h                ; IS_WINOLDAP_ACTIVE
    int     2fh
    or      ax,ax                   ; running under 3.0 derivative?
    jnz     NotRunningUnderWin
   
  ; rule out MS-DOS 5.0 task switcher
    mov     ax,4b02h                ; detect switcher
    push    bx
    push    es
    push    di
    xor     bx,bx
    mov     di,bx
    mov     es,bx
    int     2fh
    pop     di
    pop     es
    pop     bx
    or      ax,ax
    jz      NotRunningUnderWin      ; MS-DOS 5.0 task switcher found
   
  ; check for standard mode Windows 3.0
    mov     ax,1605h                ; PMODE_START
    int     2fh
    cmp     cx,-1
    jz      RunningUnderWin30Std
   
  ; check for real mode Windows 3.0
    mov     ax,1606h                ; PMODE_STOP
    int     2fh                     ; in case someone is counting
    ; Real mode Windows 3.0 is running
    mov     byte ptr [bRunningWindows], 1
    mov     byte ptr [MajorVer], 3h    
    mov     byte ptr [MinorVer], 0h        
    jmp     ExitLabel
   
  RunningUnderWin30Std:
    ; Standard mode Windows 3.0 is running
    mov     byte ptr [bRunningWindows], 1
    mov     byte ptr [MajorVer], 3h    
    mov     byte ptr [MinorVer], 0h        
    jmp     ExitLabel
   
  RunningUnderWin31:
    ; At this point: CX == 3 means Windows 3.1 enhanced mode
    ;                CX == 2 means Windows 3.1 standard mode
    mov     byte ptr [bRunningWindows], 1
    
    ; Get the version of Windows 
    mov     ax, 1600h   ; Get Enhanced-Mode Windows Installed State
    int     2Fh
    mov     byte ptr [MajorVer], al
    mov     byte ptr [MinorVer], ah
    jmp     ExitLabel
   
  RunningUnderWin30Enh:
    ; Enhanced mode Windows 3.0 is running
    mov     byte ptr [bRunningWindows], 1    
    mov     byte ptr [MajorVer], 3h    
    mov     byte ptr [MinorVer], 0h        
    jmp     ExitLabel
   
  NotRunningUnderWin:                    
    mov     byte ptr [bRunningWindows], 0
    
  ExitLabel:
  }             
  
  WinVer = (WORD) ((MajorVer << 8) + MinorVer);
} 
#endif //_DOS 

BOOL COSVersion::IsWindowsCE(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == WindowsCE);
}

BOOL COSVersion::IsWindows95(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == Windows9x &&
          lpVersionInformation->dwUnderlyingMajorVersion == 4 && 
          lpVersionInformation->dwUnderlyingMinorVersion == 0 &&
          lpVersionInformation->dwUnderlyingBuildNumber == 950);
}

BOOL COSVersion::IsWindows95SP1(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == Windows9x &&
          lpVersionInformation->dwUnderlyingMajorVersion == 4 && 
          lpVersionInformation->dwUnderlyingBuildNumber > 950 && 
          lpVersionInformation->dwUnderlyingBuildNumber <= 1080);
}

BOOL COSVersion::IsWindows95OSR2(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == Windows9x &&
          lpVersionInformation->dwUnderlyingMajorVersion == 4 && 
          lpVersionInformation->dwUnderlyingMinorVersion < 10 &&
          lpVersionInformation->dwUnderlyingBuildNumber > 1080);
}

BOOL COSVersion::IsWindows98(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == Windows9x &&
          lpVersionInformation->dwUnderlyingMajorVersion == 4 && 
          lpVersionInformation->dwUnderlyingMinorVersion == 10 &&
          lpVersionInformation->dwUnderlyingBuildNumber == 1998);
}

BOOL COSVersion::IsWindows98SP1(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == Windows9x &&
          lpVersionInformation->dwUnderlyingMajorVersion == 4 && 
          lpVersionInformation->dwUnderlyingMinorVersion == 10 &&
          lpVersionInformation->dwUnderlyingBuildNumber > 1998 &&
          lpVersionInformation->dwUnderlyingBuildNumber < 2183);
}

BOOL COSVersion::IsWindows98SE(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == Windows9x &&
          lpVersionInformation->dwUnderlyingMajorVersion == 4 && 
          lpVersionInformation->dwUnderlyingMinorVersion == 10 &&
          lpVersionInformation->dwUnderlyingBuildNumber >= 2183);
}

BOOL COSVersion::IsWindowsME(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == Windows9x &&
          lpVersionInformation->dwUnderlyingMajorVersion == 4 &&
          lpVersionInformation->dwUnderlyingMinorVersion == 90);
}

BOOL COSVersion::IsWindowsNT31(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
          lpVersionInformation->dwUnderlyingMajorVersion == 3 && 
          lpVersionInformation->dwUnderlyingMinorVersion == 10);
}

BOOL COSVersion::IsWindowsNT35(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
          lpVersionInformation->dwUnderlyingMajorVersion == 3 && 
          lpVersionInformation->dwUnderlyingMinorVersion == 50);
}

BOOL COSVersion::IsWindowsNT351(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
          lpVersionInformation->dwUnderlyingMajorVersion == 3 && 
          lpVersionInformation->dwUnderlyingMinorVersion == 51);
}

BOOL COSVersion::IsWindowsNT4(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
          lpVersionInformation->dwUnderlyingMajorVersion == 4);
}

BOOL COSVersion::IsWindows2000(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
          lpVersionInformation->dwUnderlyingMajorVersion == 5 &&
          lpVersionInformation->dwUnderlyingMinorVersion == 0);
}

BOOL COSVersion::IsWindowsXPOrWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
          lpVersionInformation->dwUnderlyingMajorVersion == 5 &&
          lpVersionInformation->dwUnderlyingMinorVersion != 0);
}

BOOL COSVersion::IsWindowsXP(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsXPPersonal(lpVersionInformation) || IsXPProfessional(lpVersionInformation);
}

WORD COSVersion::GetNTServicePackFromCSDString(LPCTSTR pszCSDVersion)
{
  WORD wServicePack = 0;
  if (_tcslen(pszCSDVersion))
  {
    //Parse out the CSDVersion string
    int i=0;      
    while (pszCSDVersion[i] != _T('\0') && !_istdigit(pszCSDVersion[i]))
      i++;
    wServicePack = (WORD) (_ttoi(&pszCSDVersion[i]));
  }

  return wServicePack;
}

#if defined(_WIN32) || defined(_WIN64)
WORD COSVersion::GetNTServicePackFromRegistry()
{
  //By default assume no service pack
  WORD wServicePack = 0;

  HKEY hCurrentVersion;
  #ifdef _WIN32
  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hCurrentVersion) == ERROR_SUCCESS)
  #else                                                                                                                                       
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion"), &hCurrentVersion) == ERROR_SUCCESS)
  #endif
  {
    BYTE byData[128];
    DWORD dwType;
    DWORD dwSize = 128;
    if (RegQueryValueEx(hCurrentVersion, _T("CSDVersion"), NULL, &dwType, byData, &dwSize) == ERROR_SUCCESS)
    {
      if (dwType == REG_SZ)
      {
        //Stored as a string on all other versions of NT
        wServicePack = GetNTServicePackFromCSDString((TCHAR*) byData);
      }
      else if (dwType == REG_DWORD)
      {
        //Handle the service pack number being stored as a DWORD which happens on NT 3.1
        wServicePack = HIBYTE((WORD) *((DWORD*) byData));
      }
    }

    //Don't forget to close the registry key we were using      
    RegCloseKey(hCurrentVersion);
  }

  return wServicePack;
}
#endif //#if defined(_WIN32) || defined(_WIN64)

#if defined(_WIN32) || defined(_WIN64) || defined (_WINDOWS)
void COSVersion::GetTerminalServicesRemoteAdminModeDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation)
{
  //Lookup the TSAppCompat registry value
  HKEY hKey = NULL;        
  #ifdef _WIN32
  LONG lResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Terminal Server"), 0, KEY_READ, &hKey);
  #else                                                                                                                         
  LONG lResult = RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Terminal Server"), &hKey);
  #endif
  if (lResult != ERROR_SUCCESS)
    return;

  DWORD dwTSAppCompat = 1;
  DWORD dwType;
  DWORD dwSize = sizeof(DWORD);
#if defined(_WIN32) || defined(_WIN64)    
  lResult = ::RegQueryValueEx(hKey, _T("TSAppCompat"), NULL, &dwType, (LPBYTE) &dwTSAppCompat, &dwSize);
#else                                                                                                     
  lResult = RegQueryValueEx(hKey, _T("TSAppCompat"), NULL, &dwType, (LPBYTE) &dwTSAppCompat, &dwSize);
#endif  
  if (lResult == ERROR_SUCCESS)
  {
    if (dwTSAppCompat == 0)
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL;
  }

  //Don't forget to free up the resource we used
  RegCloseKey(hKey);
}

void COSVersion::GetMediaCenterDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation)
{
  //Lookup the WPA\MediaCenter\Installed registry value
  HKEY hKey = NULL;        
  #ifdef _WIN32
  LONG lResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\WPA\\MediaCenter"), 0, KEY_READ, &hKey);
  #else                                                                                                                         
  LONG lResult = RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\WPA\\MediaCenter"), &hKey);
  #endif
  if (lResult != ERROR_SUCCESS)
    return;

  DWORD dwMCE = 0;
  DWORD dwType;
  DWORD dwSize = sizeof(DWORD);
#if defined(_WIN32) || defined(_WIN64)    
  lResult = ::RegQueryValueEx(hKey, _T("Installed"), NULL, &dwType, (LPBYTE) &dwMCE, &dwSize);
#else                                                                                                     
  lResult = RegQueryValueEx(hKey, _T("Installed"), NULL, &dwType, (LPBYTE) &dwMCE, &dwSize);
#endif  
  if (lResult == ERROR_SUCCESS)
  {
    if (dwMCE)
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_MEDIACENTER;
  }

  //Don't forget to free up the resource we used
  RegCloseKey(hKey);
}

void COSVersion::GetProductSuiteDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation) 
{
  //Lookup the ProductOptions registry key
  HKEY hKey = NULL;        
  #ifdef _WIN32
  LONG lResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_READ, &hKey);
  #else                                                                                                                         
  LONG lResult = RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), &hKey);
  #endif
  if (lResult != ERROR_SUCCESS)
    return;

  // Determine required size of ProductSuite buffer.
  DWORD dwType = 0;
  DWORD dwSize = 0;                   
#if defined(_WIN32) || defined(_WIN64)  
  lResult = ::RegQueryValueEx(hKey, _T("ProductSuite"), NULL, &dwType, NULL, &dwSize);
#else
  lResult = RegQueryValueEx(hKey, _T("ProductSuite"), NULL, &dwType, NULL, &dwSize);
#endif  
  if (lResult != ERROR_SUCCESS || !dwSize)
  {
    RegCloseKey(hKey);
    return;
  }

  // Allocate buffer.
  LPTSTR lpszProductSuites = (LPTSTR) new BYTE[dwSize];

  // Retrieve array of product suite strings.
#if defined(_WIN32) || defined(_WIN64)    
  lResult = ::RegQueryValueEx(hKey, _T("ProductSuite"), NULL, &dwType, (LPBYTE) lpszProductSuites, &dwSize);
#else                                                                                                     
  lResult = RegQueryValueEx(hKey, _T("ProductSuite"), NULL, &dwType, (LPBYTE) lpszProductSuites, &dwSize);
#endif  
  if (lResult != ERROR_SUCCESS || dwType != REG_MULTI_SZ)
  {
    //Don't forget to free up the resource we used
    delete [] lpszProductSuites;
    RegCloseKey(hKey);

    return;
  }

  //Search for suite name in array of strings.
  LPTSTR lpszSuite = lpszProductSuites;
  while (*lpszSuite) 
  {
    //Note we do not need to implement checks for COSVERSION_SUITE_WEBEDITION
    //as Windows Server 2003 Web Edition which supports GetVersionEx
    //using a OSVERSIONINFOEX structure.

    //NT Embedded and subsequent version of Embedded Windows supports
    //GetVersionEx using a OSVERSIONINFOEX structure, so no check for 
    //it here either

    //Also I was unable to find any documentation on what Windows NT Datacenter Server
    //puts in the ProductSuite. In Windows 2000 Datacenter it does not matter as it
    //supports GetVersionEx using a OSVERSIONINFOEX structure.

    //Turn on appropiate fields in the "wSuiteMask" bit field
    if (_tcsicmp(lpszSuite, _T("Terminal Server")) == 0) 
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_TERMINAL;
    else if ((_tcsicmp(lpszSuite, _T("SBS")) == 0) || (_tcsicmp(lpszSuite, _T("Small Business")) == 0))
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SMALLBUSINESS;
    else if (_tcsicmp(lpszSuite, _T("Enterprise")) == 0) 
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
    else if (_tcsicmp(lpszSuite, _T("Personal")) == 0) 
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_PERSONAL;

    lpszSuite += (lstrlen(lpszSuite) + 1);
  }

  //Don't forget to free up the resource we used
  delete [] lpszProductSuites;
  RegCloseKey(hKey);
}
#endif //#if defined(_WIN32) || defined(_WIN64)  || defined (_WINDOWS)
             
#if defined(_WIN64) || defined(_WIN32)             
BOOL COSVersion::IsWindows30(LPOS_VERSION_INFO /*lpVersionInformation*/)
#else
BOOL COSVersion::IsWindows30(LPOS_VERSION_INFO lpVersionInformation)
#endif //#if defined(_WIN64) || defined(_Win32             
{
#ifdef _WIN64
  return FALSE;
#elif _WIN32
  return FALSE;
#else
  return (lpVersionInformation->UnderlyingPlatform == Windows3x &&
          lpVersionInformation->dwUnderlyingMinorVersion == 0);
#endif
}

#if defined(_WIN64) || defined(_WIN32)             
BOOL COSVersion::IsWindows31(LPOS_VERSION_INFO /*lpVersionInformation*/)
#else
BOOL COSVersion::IsWindows31(LPOS_VERSION_INFO lpVersionInformation)
#endif //#if defined(_WIN64) || defined(_Win32             
{
#ifdef _WIN64
  return FALSE;
#elif _WIN32
  return FALSE;
#else
  return (lpVersionInformation->UnderlyingPlatform == Windows3x &&
          lpVersionInformation->dwUnderlyingMinorVersion == 10);
#endif
}

#if defined(_WIN64) || defined(_WIN32)            
BOOL COSVersion::IsWindows311(LPOS_VERSION_INFO /*lpVersionInformation*/)
#else
BOOL COSVersion::IsWindows311(LPOS_VERSION_INFO lpVersionInformation)
#endif //#if defined(_WIN64) || defined(_Win32             
{
#ifdef _WIN64
  return FALSE;
#elif _WIN32
  return FALSE;
#else
  return (lpVersionInformation->UnderlyingPlatform == Windows3x &&
          lpVersionInformation->dwUnderlyingMinorVersion == 11);
#endif
}

#if defined(_WINDOWS) && (!defined(_WIN32) && !defined(_WIN64))  
BOOL COSVersion::IsWindowsForWorkgroups(LPOS_VERSION_INFO lpVersionInformation)
#else
BOOL COSVersion::IsWindowsForWorkgroups(LPOS_VERSION_INFO /*lpVersionInformation*/)
#endif //#if defined(_WINDOWS) && (defined(_WIN32) || defined(_WIN64))  
{
#if defined(_WINDOWS) && (!defined(_WIN32) && !defined(_WIN64))  
  return (lpVersionInformation->UnderlyingPlatform == Windows3x &&
          WFWLoaded());
#else
  return FALSE;          
#endif //#if defined(_WINDOWS) && (!defined(_WIN32) && !defined(_WIN64))  
}


#if defined(_WIN32)
BOOL COSVersion::IsWin32sInstalled(LPOS_VERSION_INFO lpVersionInformation)
#else
BOOL COSVersion::IsWin32sInstalled(LPOS_VERSION_INFO /*lpVersionInformation*/)
#endif //#if defined(_WIN64) || defined(_Win32             
{
#ifdef _WIN64
  return FALSE;
#elif _WIN32
  return (lpVersionInformation->UnderlyingPlatform == Windows3x);
#else
  return FALSE;
#endif
}

BOOL COSVersion::IsNTPreWin2k(LPOS_VERSION_INFO lpVersionInformation)
{
  return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
          lpVersionInformation->dwUnderlyingMajorVersion <= 4);
}

BOOL COSVersion::IsNTPDC(LPOS_VERSION_INFO lpVersionInformation)
{
  return (IsNTPreWin2k(lpVersionInformation)) && 
         (lpVersionInformation->OSType == DomainController) &&
         ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_PRIMARY_DOMAIN_CONTROLLER) != 0);
}

BOOL COSVersion::IsNTStandAloneServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsNTPreWin2k(lpVersionInformation) && (lpVersionInformation->OSType == Server);
}

BOOL COSVersion::IsNTBDC(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsNTPreWin2k(lpVersionInformation) && 
         (lpVersionInformation->OSType == DomainController) && 
         ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_BACKUP_DOMAIN_CONTROLLER) != 0);
}

BOOL COSVersion::IsNTWorkstation(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsNTPreWin2k(lpVersionInformation) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsTerminalServicesInstalled(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_TERMINAL) != 0);
}

BOOL COSVersion::IsEmbedded(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_EMBEDDEDNT) != 0);
}

BOOL COSVersion::IsTerminalServicesInRemoteAdminMode(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL) != 0);
}

BOOL COSVersion::ISSmallBusinessEditionInstalled(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_SMALLBUSINESS) != 0);
}

BOOL COSVersion::IsNTEnterpriseServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return (IsNTPreWin2k(lpVersionInformation) && (lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsNTDatacenterServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsNTPreWin2k(lpVersionInformation) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0);
}

BOOL COSVersion::IsWin2000Professional(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindows2000(lpVersionInformation) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsWin2000Server(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindows2000(lpVersionInformation) && (lpVersionInformation->OSType == Server);
}

BOOL COSVersion::IsWin2000AdvancedServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindows2000(lpVersionInformation) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsWin2000DatacenterServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return (IsWindows2000(lpVersionInformation) && (lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0);
}

BOOL COSVersion::IsWin2000DomainController(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindows2000(lpVersionInformation) && (lpVersionInformation->OSType == DomainController);
}

BOOL COSVersion::IsXPPersonal(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_PERSONAL) != 0);
}

BOOL COSVersion::IsXPProfessional(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation) && (lpVersionInformation->OSType == Workstation) && !IsXPPersonal(lpVersionInformation);
}

BOOL COSVersion::IsWebWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_WEBEDITION) != 0);
}

BOOL COSVersion::IsWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation) && (lpVersionInformation->OSType == Server);
}

BOOL COSVersion::IsEnterpriseWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsDatacenterWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0);
}

BOOL COSVersion::IsDomainControllerWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation) && (lpVersionInformation->OSType == DomainController);
}

BOOL COSVersion::IsMediaCenterInstalled(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_MEDIACENTER) != 0);
}

BOOL COSVersion::IsEmulated64Bit(LPOS_VERSION_INFO /*lpVersionInformation*/)
{
#ifdef _WIN64
  return TRUE;
#else
  return FALSE;
#endif
}

BOOL COSVersion::IsUnderlying64Bit(LPOS_VERSION_INFO /*lpVersionInformation*/)
{
#ifdef _WIN64
  return TRUE;
#elif UNDER_CE
  return FALSE;
#elif _WIN32
  //Assume the worst
  BOOL bSuccess = FALSE;  
  
  //Check to see if we are running on 64 bit Windows thro WOW64
  HMODULE hKernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
  if (hKernel32)
  {
    lpfnIsWow64Process pIsWow64Process = (lpfnIsWow64Process) GetProcAddress(hKernel32, "IsWow64Process"); 
    if (pIsWow64Process)
    {
      BOOL bWow64 = FALSE;
      if (pIsWow64Process(GetCurrentProcess(), &bWow64))
        bSuccess = bWow64;
    }
  }
  return bSuccess;
#else
  return FALSE;
#endif
}

