/*
Module : DTWINVER.H
Purpose: Declaration of a comprehensive class to perform OS version detection
Created: PJN / 11-05-1996

Copyright (c) 1997 - 2015 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 

*/


////////////////////////////////// Initial setup //////////////////////////////

#ifndef __DTWINVER_H__                                          
#define __DTWINVER_H__

//Preprocessor values we need to control the various code paths
#if defined(UNDER_CE)
  #define COSVERSION_CE 1
#endif
#if defined(_DOS)
  #define COSVERSION_DOS 1
#else              
  #define COSVERSION_WIN 1
  #if defined(_WIN64)
    #define COSVERSION_WIN64 1
  #elif defined(_WIN32)
    #define COSVERSION_WIN32 1
  #else
    #define COSVERSION_WIN16 1
  #endif
#endif

#if defined(COSVERSION_DOS) || defined(COSVERSION_WIN16)  
  #define COSVERSION_WIN16_OR_DOS 1
#endif


////////////////////////////////// Includes ///////////////////////////////////
#include <winsock2.h>
#include <windows.h> 
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
#include <tchar.h>
#else
#include <ctype.h>
#include <stdlib.h>
#include <shellapi.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


////////////////////////////////// Defines ////////////////////////////////////

#define COSVERSION_SUITE_SMALLBUSINESS                        0x00000001
#define COSVERSION_SUITE_ENTERPRISE                           0x00000002
#define COSVERSION_SUITE_PRIMARY_DOMAIN_CONTROLLER            0x00000004
#define COSVERSION_SUITE_BACKUP_DOMAIN_CONTROLLER             0x00000008
#define COSVERSION_SUITE_TERMINAL                             0x00000010
#define COSVERSION_SUITE_DATACENTER                           0x00000020
#define COSVERSION_SUITE_PERSONAL                             0x00000040
#define COSVERSION_SUITE_WEBEDITION                           0x00000080
#define COSVERSION_SUITE_EMBEDDED                             0x00000100
#define COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL             0x00000200
#define COSVERSION_SUITE_UNIPROCESSOR_FREE                    0x00000400
#define COSVERSION_SUITE_UNIPROCESSOR_CHECKED                 0x00000800
#define COSVERSION_SUITE_MULTIPROCESSOR_FREE                  0x00001000
#define COSVERSION_SUITE_MULTIPROCESSOR_CHECKED               0x00002000
#define COSVERSION_SUITE_MEDIACENTER                          0x00004000
#define COSVERSION_SUITE_TABLETPC                             0x00008000
#define COSVERSION_SUITE_STARTER_EDITION                      0x00010000
#define COSVERSION_SUITE_R2_EDITION                           0x00020000
#define COSVERSION_SUITE_COMPUTE_SERVER                       0x00040000
#define COSVERSION_SUITE_STORAGE_SERVER                       0x00080000
#define COSVERSION_SUITE_SECURITY_APPLIANCE                   0x00100000
#define COSVERSION_SUITE_BACKOFFICE                           0x00200000
#define COSVERSION_SUITE_ULTIMATE                             0x00400000
#define COSVERSION_SUITE_N                                    0x00800000
#define COSVERSION_SUITE_HOME_BASIC                           0x01000000
#define COSVERSION_SUITE_HOME_PREMIUM                         0x02000000
#define COSVERSION_SUITE_HYPERV_TOOLS                         0x04000000
#define COSVERSION_SUITE_BUSINESS                             0x08000000
#define COSVERSION_SUITE_HOME_SERVER                          0x10000000
#define COSVERSION_SUITE_SERVER_CORE                          0x20000000
#define COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MANAGEMENT 0x40000000
#define COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MESSAGING  0x80000000

#define COSVERSION_SUITE2_ESSENTIAL_BUSINESS_SERVER_SECURITY  0x00000001
#define COSVERSION_SUITE2_CLUSTER_SERVER                      0x00000002
#define COSVERSION_SUITE2_SMALLBUSINESS_PREMIUM               0x00000004
#define COSVERSION_SUITE2_STORAGE_EXPRESS_SERVER              0x00000008
#define COSVERSION_SUITE2_STORAGE_WORKGROUP_SERVER            0x00000010
#define COSVERSION_SUITE2_STANDARD                            0x00000020
#define COSVERSION_SUITE2_E                                   0x00000040
#define COSVERSION_SUITE2_PROFESSIONAL                        0x00000080
#define COSVERSION_SUITE2_FOUNDATION                          0x00000100
#define COSVERSION_SUITE2_MULTIPOINT                          0x00000200
#define COSVERSION_SUITE2_HYPERV_SERVER                       0x00000400
#define COSVERSION_SUITE2_HOME_SERVER_PREMIUM                 0x00000800
#define COSVERSION_SUITE2_STORAGE_SERVER_ESSENTIALS           0x00001000
#define COSVERSION_SUITE2_PRERELEASE                          0x00002000
#define COSVERSION_SUITE2_EVALUATION                          0x00004000
#define COSVERSION_SUITE2_PREMIUM                             0x00008000
#define COSVERSION_SUITE2_MULTIPOINT_SERVER_PREMIUM           0x00010000
#define COSVERSION_SUITE2_THINPC                              0x00020000
#define COSVERSION_SUITE2_AUTOMOTIVE                          0x00040000
#define COSVERSION_SUITE2_CHINA                               0x00080000
#define COSVERSION_SUITE2_SINGLE_LANGUAGE                     0x00100000
#define COSVERSION_SUITE2_WIN32S                              0x00200000
#define COSVERSION_SUITE2_WINDOWS812012R2UPDATE               0x00400000
#define COSVERSION_SUITE2_CORECONNECTED                       0x00800000
#define COSVERSION_SUITE2_EDUCATION                           0x01000000
#define COSVERSION_SUITE2_INDUSTRY                            0x02000000
#define COSVERSION_SUITE2_CORE                                0x04000000
#define COSVERSION_SUITE2_STUDENT                             0x08000000
#define COSVERSION_SUITE2_MOBILE                              0x10000000
#define COSVERSION_SUITE2_IOT                                 0x20000000
#define COSVERSION_SUITE2_S                                   0x40000000
#define COSVERSION_SUITE2_NANO_SERVER                         0x80000000

#define COSVERSION_SUITE3_CLOUD_STORAGE_SERVER                0x00000001
#define COSVERSION_SUITE3_ARM64_SERVER                        0x00000002
#define COSVERSION_SUITE3_PPI_PRO                             0x00000004
#define COSVERSION_SUITE3_CONNECTED_CAR                       0x00000008
#define COSVERSION_SUITE3_HANDHELD                            0x00000010
#define COSVERSION_SUITE3_CLOUD_HOST_INFRASTRUCTURE_SERVER    0x00000020


////////////////////////////////// Classes ////////////////////////////////////

class COSVersion
{
public:
//Enums
  enum OS_PLATFORM
  {
    UnknownOSPlatform = 0,
    Dos = 1,
    Windows3x = 2,
    Windows9x = 3,
    WindowsNT = 4,
    WindowsCE = 5,
  };

  enum OS_TYPE
  {
    UnknownOSType = 0,
    Workstation = 1,
    Server = 2,
    DomainController = 3,
  };

  enum PROCESSOR_TYPE
  {
    UNKNOWN_PROCESSOR = 0,
    IA32_PROCESSOR = 1,
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
  
//Defines
  typedef struct _OS_VERSION_INFO
  {
    //What version of OS is being emulated
    DWORD          dwEmulatedMajorVersion;
    DWORD          dwEmulatedMinorVersion;
    DWORD          dwEmulatedBuildNumber;
    OS_PLATFORM    EmulatedPlatform;
  #if !defined(COSVERSION_CE)
    PROCESSOR_TYPE EmulatedProcessorType;       //The emulated processor type
  #endif
  #if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  #if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
    TCHAR          szEmulatedCSDVersion[128];
  #endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  #else
    char           szEmulatedCSDVersion[128];
  #endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)  
    WORD           wEmulatedServicePackMajor;
    WORD           wEmulatedServicePackMinor;

    //What version of OS is really running                 
    DWORD          dwUnderlyingMajorVersion;
    DWORD          dwUnderlyingMinorVersion;
    DWORD          dwUnderlyingBuildNumber;
    OS_PLATFORM    UnderlyingPlatform;   
  #if !defined(COSVERSION_CE)
    PROCESSOR_TYPE UnderlyingProcessorType;     //The underlying processor type
  #endif //#if !defined(COSVERSION_CE)
  #if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64) 
  #if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
    TCHAR          szUnderlyingCSDVersion[128];
  #endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  #else  
    char           szUnderlyingCSDVersion[128];
  #endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)  
    WORD           wUnderlyingServicePackMajor;
    WORD           wUnderlyingServicePackMinor;
    DWORD          dwSuiteMask;                 //Bitmask of various OS suites
    DWORD          dwSuiteMask2;                //Second set of Bitmask of various OS suites
    DWORD          dwSuiteMask3;                //Third set of Bitmask of various OS suites
    OS_TYPE        OSType;                      //The basic OS type

  #if defined(COSVERSION_CE)
    TCHAR szOEMInfo[256];
    TCHAR szPlatformType[256];
  #endif //#if defined(COSVERSION_CE)
  } OS_VERSION_INFO, *POS_VERSION_INFO, FAR *LPOS_VERSION_INFO;

//Constructors / Destructors
  COSVersion(); 
  ~COSVersion();

//Methods:
  BOOL GetVersion(LPOS_VERSION_INFO lpVersionInformation);
                                 
//Please note that the return values for the following group of functions 
//are mutually exclusive for example if you are running on 
//95 OSR2 then IsWindows95 will return FALSE etc.
  BOOL IsWindows30(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows31(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows311(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsForWorkgroups(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsCE(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows95(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows95SP1(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows95B(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows95C(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows98(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows98SP1(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows98SE(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsME(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsNT31(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsNT35(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsNT351(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsNT4(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows2000(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsXPOrWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsVistaOrWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows7OrWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows8OrWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows8Point1OrWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows10OrWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

//Returns the various flavours of the "os" that is installed. Note that these
//functions are not completely mutually exlusive
  BOOL IsWin32sInstalled(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNTPreWin2k(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsNTWorkstation(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsNTStandAloneServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsNTPDC(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsNTBDC(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsNTEnterpriseServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsNTDatacenterServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsWindows2000Professional(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows2000Server(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows2000AdvancedServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows2000DatacenterServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows2000DomainController(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsWindowsXP(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsXPPersonal(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsXPProfessional(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsVista(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows7(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows8(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows8Point1(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows8Point1Update(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindows10(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsWebWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsStandardWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsEnterpriseWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDatacenterWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDomainControllerWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsWebWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsStandardWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsEnterpriseWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDatacenterWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDomainControllerWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsWebWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsStandardWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsEnterpriseWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDatacenterWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDomainControllerWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsWebWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsStandardWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsEnterpriseWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDatacenterWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDomainControllerWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsWebWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsServer2012R2Update(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsStandardWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsEnterpriseWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDatacenterWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDomainControllerWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsWebWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsStandardWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsEnterpriseWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDatacenterWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDomainControllerWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  
  BOOL IsHomeBasic(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsHomeBasicPremium(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsBusiness(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsProfessional(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEnterprise(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsUltimate(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsPersonal(LPOS_VERSION_INFO lpVersionInformation);

  BOOL IsWebWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsStandardWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsEnterpriseWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDatacenterWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsDomainControllerWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);

  BOOL IsTerminalServices(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsSmallBusinessServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsSmallBusinessServerPremium(LPOS_VERSION_INFO lpVersionInformation);
  BOOL Is64Bit(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsEmbedded(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsTerminalServicesInRemoteAdminMode(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsMediaCenter(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsTabletPC(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsStarterEdition(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsComputeClusterServerEdition(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsHomeServerEdition(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsHomeServerPremiumEdition(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsSecurityAppliance(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsBackOffice(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNEdition(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEEdition(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsHyperVTools(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsHyperVServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsServerCore(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsMultiPointServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsServerFoundation(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsUniprocessorFree(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsUniprocessorChecked(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsMultiprocessorFree(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsMultiprocessorChecked(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEssentialBusinessServerManagement(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEssentialBusinessServerMessaging(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEssentialBusinessServerSecurity(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsClusterServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsStorageServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEnterpriseStorageServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsExpressStorageServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsStandardStorageServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWorkgroupStorageServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEssentialsStorageServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsPreRelease(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEvaluation(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsRT(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsWindowsCENET(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsPremium(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindowsEmbeddedCompact(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying);
  BOOL IsMultipointServerPremiumEdition(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsThinPC(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsAutomotive(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsChina(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsSingleLanguage(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsWindows8Point1Or2012R2Update(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsCoreConnected(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsEducation(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsIndustry(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsCore(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsStudent(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsMobile(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsIoT(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsCloudHostInfrastructureServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsSEdition(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsNanoServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsCloudStorageServer(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsARM64Server(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsPPIPro(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsConnectedCar(LPOS_VERSION_INFO lpVersionInformation);
  BOOL IsHandheld(LPOS_VERSION_INFO lpVersionInformation);

protected:
//Defines / typedefs
#if defined(COSVERSION_WIN16_OR_DOS)
  #define CPEX_DEST_STDCALL        0x00000000L
  #define HKEY32                   DWORD                                                               
  #define HKEY_LOCAL_MACHINE       (( HKEY32 ) 0x80000002 )    
  #define TCHAR                    char  
  #define WCHAR                    unsigned short
  #define _T
  #define _tcsicmp                 strcmpi
  #define _tcscpy                  strcpy
  #define _tcslen                  strlen
  #define _istdigit                isdigit
  #define _ttoi                    atoi
  #define _tcsupr                  strupr
  #define _tcsstr                  strstr
  #define _stscanf                 sscanf
  #define LPCTSTR                  LPCSTR
  #define ERROR_CALL_NOT_IMPLEMENTED  120
  #define REG_DWORD                ( 4 )                                                               
  #define REG_MULTI_SZ             ( 7 )
  #define VER_PLATFORM_WIN32s             0
  #define VER_PLATFORM_WIN32_WINDOWS      1
  #define VER_PLATFORM_WIN32_NT           2
  #define VER_PLATFORM_WIN32_CE           3
                      
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
  static BOOL WFWLoaded();                                     
  LONG RegQueryValueEx(HKEY32 hKey, LPSTR lpszValueName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE  lpbData, LPDWORD lpcbData);

//Function Prototypes
  typedef DWORD (FAR PASCAL  *lpfnLoadLibraryEx32W) (LPCSTR, DWORD, DWORD);
  typedef BOOL  (FAR PASCAL  *lpfnFreeLibrary32W)   (DWORD);
  typedef DWORD (FAR PASCAL  *lpfnGetProcAddress32W)(DWORD, LPCSTR);
  typedef DWORD (FAR __cdecl *lpfnCallProcEx32W)    (DWORD, DWORD, DWORD, DWORD, ...);
  typedef WORD  (FAR PASCAL  *lpfnWNetGetCaps)      (WORD);
  typedef BOOL  (FAR PASCAL  *lpfnGetProductInfo)   (DWORD, DWORD, DWORD, DWORD, PDWORD);

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
  DWORD                 m_lpfnGetProductInfo;
#endif //#if defined(COSVERSION_WIN16_OR_DOS)

#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64) 
//Function Prototypes
  typedef BOOL (WINAPI *lpfnIsWow64Process)(HANDLE, PBOOL);  
  typedef void (WINAPI *lpfnGetNativeSystemInfo)(LPSYSTEM_INFO);
  typedef BOOL (WINAPI *lpfnGetProductInfo)(DWORD, DWORD, DWORD, DWORD, DWORD*);
#endif

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

  typedef struct _OSVERSIONINFOEXW 
  {
    DWORD  dwOSVersionInfoSize;
    DWORD  dwMajorVersion;
    DWORD  dwMinorVersion;
    DWORD  dwBuildNumber;
    DWORD  dwPlatformId;
    WCHAR  szCSDVersion[128];
    WORD   wServicePackMajor;
    WORD   wServicePackMinor;
    WORD   wSuiteMask;
    BYTE  wProductType;
    BYTE  wReserved;
  } RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;
                                        
#if defined(_WIN32) || defined(_WINDOWS)                                        
//Function Prototypes                  
  typedef BOOL (WINAPI *lpfnGetVersionEx)(LPOSVERSIONINFO);  
  typedef DWORD (WINAPI *lpfnGetVersion)();
  typedef LONG (WINAPI *lpfnRtlGetVersion)(PRTL_OSVERSIONINFOEXW);
#endif  

//Methods
#if defined(COSVERSION_DOS)
  BOOL GetInfoBySpawingWriteVer(COSVersion::LPOS_VERSION_INFO lpVersionInformation);
  void GetWinInfo();
#else
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  void GetNTSP6aDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bUpdateEmulatedAlso);
  void GetXPSP1aDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bUpdateEmulatedAlso);
  OS_TYPE GetNTOSTypeFromRegistry();
  OS_TYPE GetNTOSTypeFromRegistry(HKEY hKeyProductOptions);
  void GetNTOSTypeFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bOnlyUpdateDCDetails);
  void GetBingEditionIDFromRegistry(LPOS_VERSION_INFO lpVersionInformation);
  void GetProductSuiteDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation);
  void GetTerminalServicesRemoteAdminModeDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation);
  void GetMediaCenterDetails(LPOS_VERSION_INFO lpVersionInformation);
  void GetProductInfo(LPOS_VERSION_INFO lpVersionInformation);
  void GetTabletPCDetails(LPOS_VERSION_INFO lpVersionInformation);
  void GetStarterEditionDetails(LPOS_VERSION_INFO lpVersionInformation);
  void GetR2Details(LPOS_VERSION_INFO lpVersionInformation);
  void GetNTHALDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation);
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#endif //#if defined(COSVERSION_DOS)

#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  static BOOL GetNTCSDVersionFromRegistry(HKEY hKeyCurrentVersion, LPOS_VERSION_INFO lpVersionInformation, BOOL bEmulated);
  static DWORD GetNTCurrentBuildFromRegistry(HKEY hKeyCurrentVersion);
  static BOOL GetNTCurrentBuildFromRegistry(DWORD& dwCurrentBuild);
  static BOOL GetNTCurrentVersionFromRegistry(HKEY hKeyCurrentVersion, DWORD& dwMajorVersion, DWORD& dwMinorVersion);
  static BOOL GetNTRTLVersion(LPOS_VERSION_INFO lpVersionInformation);
  static BOOL GetWindows8Point1Or2012R2Update(LPOS_VERSION_INFO lpVersionInformation);
  void _GetVersion(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber, TCHAR* szCSDVersion, WORD wServicePackMajor, 
                   WORD wServicePackMinor, DWORD dwPlatformId, BOOL bOnlyUpdateDCDetails, LPOS_VERSION_INFO lpVersionInformation);
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)   

  static BOOL IsWindows95SP1(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber);
  static BOOL IsWindows95B(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber);
  static BOOL IsWindows95C(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber);
  static BOOL IsWindows98(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber);
  static BOOL IsWindows98SP1(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber);
  static BOOL IsWindows98SE(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber);
  static BOOL IsWindowsME(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber);
  static WORD GetNTServicePackFromCSDString(LPCTSTR pszCSDVersion);
  static PROCESSOR_TYPE MapProcessorArchitecture(WORD wProcessorArchitecture);
  static DWORD MapWin32SuiteMask(WORD wSuiteMask);
  static OS_TYPE MapWin32ProductType(WORD wProductType);
  static OS_PLATFORM MapWin32PlatformId(DWORD dwPlatformId);
  BOOL GetProcessorType(LPOS_VERSION_INFO lpVersionInformation);

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
  BOOL GetVersion(DWORD& dwVersion);
  BOOL GetVersionEx(LPOSVERSIONINFO lpVersionInfo);
  BOOL GetVersionEx(LPOSVERSIONINFOEX lpVersionInfo);
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
};

#endif //__DTWINVER_H__