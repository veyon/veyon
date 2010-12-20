/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 Ultr@VNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

/*
Module : DTWINVER.H
Purpose: Interface of a function to perform
         version detection on OS
Created: PJN / 11-05-1996


Copyright (c) 1997 - 2004 by PJ Naughter.  (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 

*/

#ifndef __DTWINVER_H__                                          


////////////////////////////////// Defines ////////////////////////////////////

#define COSVERSION_SUITE_SMALLBUSINESS             0x00000001
#define COSVERSION_SUITE_ENTERPRISE                0x00000002
#define COSVERSION_SUITE_PRIMARY_DOMAIN_CONTROLLER 0x00000004
#define COSVERSION_SUITE_BACKUP_DOMAIN_CONTROLLER  0x00000008
#define COSVERSION_SUITE_TERMINAL                  0x00000010
#define COSVERSION_SUITE_DATACENTER                0x00000020
#define COSVERSION_SUITE_PERSONAL                  0x00000040
#define COSVERSION_SUITE_WEBEDITION                0x00000080
#define COSVERSION_SUITE_EMBEDDEDNT                0x00000100
#define COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL  0x00000200
#define COSVERSION_SUITE_UNIPROCESSOR_FREE         0x00000400
#define COSVERSION_SUITE_UNIPROCESSOR_CHECKED      0x00000800
#define COSVERSION_SUITE_MULTIPROCESSOR_FREE       0x00001000
#define COSVERSION_SUITE_MULTIPROCESSOR_CHECKED    0x00002000
#define COSVERSION_SUITE_MEDIACENTER               0x00004000



////////////////////////////////// Classes ////////////////////////////////////

class COSVersion
{
public:
//Enums
  enum OS_PLATFORM
  {
    Dos = 0,
    Windows3x = 1,
    Windows9x = 2,
    WindowsNT = 3,
    WindowsCE = 4,
  };

  enum OS_TYPE
  {
    Workstation = 0,
    Server = 1,
    DomainController = 2,
  };

  enum PROCESSOR_TYPE
  {
    UNKNOWN_PROCESSOR = 0,
    INTEL_PROCESSOR = 1,
    MIPS_PROCESSOR = 2,
    ALPHA_PROCESSOR = 3,
    PPC_PROCESSOR = 4,
    IA64_PROCESSOR = 5,
    AMD64_PROCESSOR = 6,
    ALPHA64_PROCESSOR = 7,
    MSIL_PROCESSOR = 8,
    ARM_PROCESSOR = 9,
    SHX_PROCESSOR = 10,
  };

//defines
  typedef struct _OS_VERSION_INFO
  {
  #ifndef UNDER_CE
    //What version of OS is being emulated
    DWORD          dwEmulatedMajorVersion;
    DWORD          dwEmulatedMinorVersion;
    DWORD          dwEmulatedBuildNumber;
    OS_PLATFORM    EmulatedPlatform;
    PROCESSOR_TYPE EmulatedProcessorType; //The emulated processor type
  #ifdef _WIN32                    
    TCHAR          szEmulatedCSDVersion[128];
  #else
    char           szEmulatedCSDVersion[128];
  #endif  
    WORD           wEmulatedServicePackMajor;
    WORD           wEmulatedServicePackMinor;
  #endif

    //What version of OS is really running                 
    DWORD       dwUnderlyingMajorVersion;
    DWORD       dwUnderlyingMinorVersion;
    DWORD       dwUnderlyingBuildNumber;
    OS_PLATFORM UnderlyingPlatform;   
  #ifdef _WIN32                      
    TCHAR       szUnderlyingCSDVersion[128];
  #else  
    char        szUnderlyingCSDVersion[128];
  #endif  
    WORD    wUnderlyingServicePackMajor;
    WORD    wUnderlyingServicePackMinor;

    DWORD          dwSuiteMask;             //Bitmask of various OS suites
    OS_TYPE        OSType;                  //The basic OS type
  #ifndef UNDER_CE
    PROCESSOR_TYPE UnderlyingProcessorType; //The underlying processor type
  #endif

  #ifdef UNDER_CE
    TCHAR szOEMInfo[256];
    TCHAR szPlatformType[256];
  #endif
  } OS_VERSION_INFO, *POS_VERSION_INFO, FAR *LPOS_VERSION_INFO;

//Constructors / Destructors
  COSVersion(); 
  ~COSVersion();

//Methods:
  BOOL GetVersion(LPOS_VERSION_INFO lpVersionInformation);
                                 
//Please note that the return values for all the following functions 
//are mutually exclusive for example if you are running on 
//95 OSR2 then IsWindows95 will return FALSE
  BOOL IsWindows30(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows31(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows311(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsForWorkgroups(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsCE(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows95(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows95SP1(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows95OSR2(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows98(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows98SP1(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows98SE(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsME(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsNT31(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsNT35(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsNT351(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsNT4(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows2000(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsXPOrWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation);

//Returns the various flavours of the "os" that is installed. Note that these
//functions are not completely mutually exlusive
  BOOL IsWin32sInstalled(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNTPreWin2k(LPOS_VERSION_INFO lpVersionInformation);

  BOOL IsNTWorkstation(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNTStandAloneServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNTPDC(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNTBDC(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNTEnterpriseServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNTDatacenterServer(LPOS_VERSION_INFO lpVersionInformation);

  BOOL IsWin2000Professional(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWin2000Server(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWin2000AdvancedServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWin2000DatacenterServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWin2000DomainController(LPOS_VERSION_INFO lpVersionInformation);

  BOOL IsXPPersonal(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsXPProfessional(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsXP(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWebWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEnterpriseWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsDatacenterWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsDomainControllerWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation);

  BOOL IsTerminalServicesInstalled(LPOS_VERSION_INFO lpVersionInformation);
  BOOL ISSmallBusinessEditionInstalled(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEmulated64Bit(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsUnderlying64Bit(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEmbedded(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsTerminalServicesInRemoteAdminMode(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsMediaCenterInstalled(LPOS_VERSION_INFO lpVersionInformation);

protected:
//Defines / typedefs
#if (defined(_WINDOWS) || defined(_DOS)) && (!defined(_WIN32) && !defined(_WIN64))  
  #define CPEX_DEST_STDCALL        0x00000000L
  #define HKEY32                   DWORD                                                               
  #define HKEY_LOCAL_MACHINE       (( HKEY32 ) 0x80000002 )    
  #define TCHAR                    char
  #define _T
  #define _tcsicmp                 strcmpi
  #define _tcscpy                  strcpy
  #define _tcslen                  strlen
  #define _istdigit                isdigit
  #define _ttoi                    atoi
  #define _tcsupr                  strupr
  #define _tcsstr                  strstr
  #define LPCTSTR                  LPCSTR
  #define ERROR_CALL_NOT_IMPLEMENTED  120
  #define REG_DWORD                ( 4 )                                                               
  #define REG_MULTI_SZ             ( 7 )
  #define VER_PLATFORM_WIN32s             0
  #define VER_PLATFORM_WIN32_WINDOWS      1
  #define VER_PLATFORM_WIN32_NT           2
  #define VER_PLATFORM_WIN32_CE           3
#endif                      
                      
#if defined(_WINDOWS) && !defined(_WIN32) && !defined(_WIN64)  
//Defines / Macros
  #define LPTSTR LPSTR

  typedef struct OSVERSIONINFO
  { 
    DWORD dwOSVersionInfoSize; 
    DWORD dwMajorVersion; 
    DWORD dwMinorVersion; 
    DWORD dwBuildNumber; 
    DWORD dwPlatformId; 
    char szCSDVersion[128]; 
  } OSVERSIONINFO, *POSVERSIONINFO, FAR *LPOSVERSIONINFO; 

  typedef struct _SYSTEM_INFO 
  {
    union 
    {
      DWORD dwOemId;          // Obsolete field...do not use
      struct 
      {
        WORD wProcessorArchitecture;
        WORD wReserved;
      };
    };
    DWORD dwPageSize;
    LPVOID lpMinimumApplicationAddress;
    LPVOID lpMaximumApplicationAddress;
    DWORD dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD wProcessorLevel;
    WORD wProcessorRevision;
  } SYSTEM_INFO, *LPSYSTEM_INFO;

  #define PROCESSOR_ARCHITECTURE_INTEL            0
  #define PROCESSOR_ARCHITECTURE_MIPS             1
  #define PROCESSOR_ARCHITECTURE_ALPHA            2
  #define PROCESSOR_ARCHITECTURE_PPC              3
  #define PROCESSOR_ARCHITECTURE_SHX              4
  #define PROCESSOR_ARCHITECTURE_ARM              5
  #define PROCESSOR_ARCHITECTURE_IA64             6
  #define PROCESSOR_ARCHITECTURE_ALPHA64          7
  #define PROCESSOR_ARCHITECTURE_MSIL             8
  #define PROCESSOR_ARCHITECTURE_AMD64            9
  #define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64    10
  #define PROCESSOR_ARCHITECTURE_UNKNOWN          0xFFFF
  
//Methods
  DWORD GetVersion();
  BOOL GetVersionEx(LPOSVERSIONINFO lpVersionInfo);
  void GetProcessorType(LPOS_VERSION_INFO lpVersionInformation);
  LONG RegQueryValueEx(HKEY32 hKey, LPSTR  lpszValueName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE  lpbData, LPDWORD lpcbData);
  static BOOL WFWLoaded();                                     

//Function Prototypes
  typedef DWORD (FAR PASCAL  *lpfnLoadLibraryEx32W) (LPCSTR, DWORD, DWORD);
  typedef BOOL  (FAR PASCAL  *lpfnFreeLibrary32W)   (DWORD);
  typedef DWORD (FAR PASCAL  *lpfnGetProcAddress32W)(DWORD, LPCSTR);
  typedef DWORD (FAR __cdecl *lpfnCallProcEx32W)    (DWORD, DWORD, DWORD, DWORD, ...);
  typedef WORD  (FAR PASCAL  *lpfnWNetGetCaps)      (WORD);

//Member variables
  lpfnLoadLibraryEx32W  m_lpfnLoadLibraryEx32W;
  lpfnFreeLibrary32W    m_lpfnFreeLibrary32W;
  lpfnGetProcAddress32W m_lpfnGetProcAddress32W;
  lpfnCallProcEx32W     m_lpfnCallProcEx32W;
  DWORD                 m_hAdvApi32;    
  DWORD                 m_hKernel32;
  DWORD                 m_lpfnRegQueryValueExA;
  DWORD                 m_lpfnGetVersionExA;
  DWORD                 m_lpfnGetVersion;
  DWORD                 m_lpfnGetSystemInfo;
  DWORD                 m_lpfnGetNativeSystemInfo;
#endif //defined(_WINDOWS) && (!defined(_WIN32) && !defined(_WIN64))

#ifdef _WIN32
//Function Prototypes
  typedef BOOL (WINAPI *lpfnIsWow64Process)(HANDLE, PBOOL);  
  typedef void (WINAPI *lpfnGetNativeSystemInfo)(LPSYSTEM_INFO);
#endif

#if defined(_WIN32) || defined(_WINDOWS)
//Defines / Macros
  typedef struct OSVERSIONINFOEX 
  {  
    DWORD dwOSVersionInfoSize;  
    DWORD dwMajorVersion;  
    DWORD dwMinorVersion;  
    DWORD dwBuildNumber;  
    DWORD dwPlatformId;  
    TCHAR szCSDVersion[128];  
    WORD wServicePackMajor;  
    WORD wServicePackMinor;  
    WORD wSuiteMask;  
    BYTE wProductType;  
    BYTE wReserved;
  } OSVERSIONINFOEX, *POSVERSIONINFOEX, *LPOSVERSIONINFOEX;
                  
//Function Prototypes                  
  typedef BOOL (WINAPI *lpfnGetVersionEx) (LPOSVERSIONINFO);  
#endif  

#ifndef _DOS         
//Methods
  void GetNTOSTypeFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bOnlyUpdateDCDetails);
  void GetProductSuiteDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation) ;
  void GetTerminalServicesRemoteAdminModeDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation);
  void GetMediaCenterDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation);
  void GetNTSP6aDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bUpdateEmulatedAlso);
  void GetXPSP1aDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bUpdateEmulatedAlso);
  void GetNTHALDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation);
#else
  BOOL GetInfoBySpawingWriteVer(COSVersion::LPOS_VERSION_INFO lpVersionInformation);
  void GetWinInfo();
#endif

#if defined(_WIN32) || defined(_WIN64)
  WORD GetNTServicePackFromRegistry();
#endif    
  WORD GetNTServicePackFromCSDString(LPCTSTR pszCSDVersion);
};

#endif //__DTWINVER_H__