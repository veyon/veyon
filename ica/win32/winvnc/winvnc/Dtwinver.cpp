/*
Module : Dtwinver.cpp
Purpose: Implementation of a comprehensive class to perform OS version detection
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
                          9. Generic thunk code now uses CallProcEx32W
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
                          Please note that if you intend deploying Dtwinver then you must now ship the writever.exe file
                          in addition to linking in the Dtwinver code into your application. Also this utilty needs
                          to be in the path or current directory when the Dtwinver code is executing. Thanks to Chaz Angell
                          for prompted me into finding a solution for this last major item missing in Dtwinver.
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
                          embedded version of Windows, you will probably compile in the Dtwinver code rather than ship the 
                          sample binaries I include in the Dtwinver download.
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
                          3. Dtwinver now returns the processor architecture via a call to GetSystemInfo or GetNativeSystemInfo. 
                          This is used to differentiate between 64 Bit Windows on Itanium and AMD64 processors.
                          4. Renamed the global preprocesor defines used by Dtwinver to use more unique names
                          5. Added make files and binaries for AMD64 processors
         PJN / 26-06-2005 1. INTEL_PROCESSOR enum is now more correctly know as IA32 which means x86 32 bit.
                          2. Fixed a bug where the OS_VERSION_INFO parameter was not be zero set in the function 
                          COSVersion::GetVersion. Thanks to "Remio" for reporting this issue.
         PJN / 29-06-2005 1. Added support for detecting if code is running on Tablet PC Edition of Windows.
                          2. Code now uses GetSystemMetrics call in preference to direct registry calls to detect Media Center 
                          Edition of Windows.
                          3. Renamed the functions which differentiate between the different OEM Service Release versions of Windows
                          95. They are OSR 2[.1] = Win95B, OSR2.5 = Win95C. For the gory details please see the KB article at 
                          http://support.microsoft.com/kb/q158238/
         PJN / 22-07-2005 1. Now includes support for Windows Vista,  formerly Windows codename "Longhorn"
         PJN / 06-09-2005 1. Renamed some of the functions which detect Longhorn Server. Previously the functions were 
                          incorrectly referring to Vista Server. Vista is the name of the Windows Client. As of yet, a product 
                          name has not been picked for Longhorn Server.
                          2. Now includes support for Windows Server 2003 R2.
                          3. Now includes support for Windows XP Starter Edition and any future versions such as Windows Vista 
                          Starter Edition.
         PJN / 17-05-2006 1. Updated copyright details.
                          2. Updated documentation to use the same style as the web site.
                          3. Updated documentation on version numbers which XP Media Center can have.
                          4. Added support for Windows Server 2003, Computer Cluster Edition
                          5. Added support for Windows Storage Server 2003 R2
                          6. Added support for detecting if Windows is installed in a "Security Application" mode
                          7. Added support for detecting BackOffice components are installed
                          7. Provided Visual Studio 2005 solutions to build DtWinver with.
         PJN / 21-09-2006 1. Included support for detecting all the different types of Vista SKU's, namely:
                          Windows Vista Starter Edition, Windows Vista Home Basic N, Windows Vista Home Basic,
                          Windows Vista Home Premium, Windows Vista Business N, Windows Vista Business,
                          Windows Vista Enterprise, and Windows Vista Ultimate.
                          2. Fixed a bug in the displaying of unknown versions of Windows in the sample app
         PJN / 18-05-2007 1. Updated copyright details.
                          2. Updated code to refer to Windows Server 2008 which was formerly known as Windows Server codename "Longhorn"
         PJN / 20-06-2007 1. Include support for detecting Windows Home Server. Thanks to Petr Stejskal for suggesting this update.
         PJN / 14-06-2008 1. Updated copyright details
                          2. Code now compiles cleanly using Code Analysis (/analyze)
                          3. Added support for Vista Home Premium N
                          4. Added support for Vista Business N
                          5. Added support for Vista Enterprise N
                          6. Added support for Vista Ultimate N
                          7. Added support for Windows Core Server
                          8. Added support for Windows Hyper-V
                          9. Added support for Windows Essential Business Server Management Server
                          10. Added support for Windows Essential Business Server Messaging Server
                          11. Added support for Windows Essential Business Server Security Server
                          12. Added support for Windows Storage Server Enterprise Edition
                          13. Added support for Windows Storage Server Express Edition
                          14. Added support for Windows Storage Server Standard Edition
                          15. Added support for Windows Storage Server Workgroup Edition     
                          16. Added support for Windows Cluster Server Edition                     
                          17. Added support for Windows Business Server Premium Edition
         PJN / 15-12-2008 1. Added support for detecting Windows 7
         PJN / 05-07-2010 1. Updated copyright details
                          2. Added a IsWindowsVistaOrWindowsServer2008 method        
                          3. Added a IsWindows7OrWindowsServer2008R2 method
                          4. Reworked the logic in all the Windows 2008 methods to use IsWindowsVistaOrWindowsServer2008 instead of 
                          IsWindowsServer2008. Thanks to Matt Fox for reporting this issue.
                          5. Added comprehensive support for Windows 2008 R2
                          6. Reworked the GetInfoBySpawingWriteVer method to work correctly on OSes which use UAC. The code now uses
                          _tempnam which places the temporary file in the "TMP" directory.
                          7. Fixed a bug in GetInfoBySpawingWriteVer where it incorrectly parsed the dwSuiteMask values.
                          8. Extended WriteVer and GetInfoBySpawingWriteVer to also update dwSuiteMask2
                          9. Added support for the following Product types: PRODUCT_ENTERPRISE_E, PRODUCT_HOME_BASIC_E, 
                          PRODUCT_HOME_PREMIUM_E, PRODUCT_PROFESSIONAL, PRODUCT_PROFESSIONAL_E, PRODUCT_PROFESSIONAL_N,
                          PRODUCT_SERVER_FOR_SMALLBUSINESS, PRODUCT_SERVER_FOR_SMALLBUSINESS_V, PRODUCT_SERVER_FOUNDATION,
                          PRODUCT_SOLUTION_EMBEDDEDSERVER, PRODUCT_STARTER_E, PRODUCT_STARTER_N, PRODUCT_ULTIMATE_E. This means that
                          the code now fully supports Windows 7 Professional, Windows E Edition (The version of Windows 7 that was to
                          be released in Europe without Internet Explorer 8), Windows Server 2008 R2 Foundation Server and 
                          Windows Multipoint Server 2010.
                          10. Changed some of the names of the class methods for overall consistency
         PJN / 08-08-2010 1. The code now differentiates between having the Hyper-V tools installed and the actual Hyper-V OS. Thanks
                          to Scott Pawloski for reporting this issue.
         PJN / 26-06-2011 1. Updated copyright details.
                          2. Added support for the following Product types: PRODUCT_HOME_SERVER_PREMIUM, 
                          PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE, PRODUCT_CLUSTER_SERVER_V, PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE,
                          PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE, PRODUCT_SB_SOLUTION_SERVER, PRODUCT_EMBEDDED, 
                          PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT, PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE, PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL,
                          PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC, PRODUCT_STORAGE_EXPRESS_SERVER_CORE, PRODUCT_STORAGE_STANDARD_SERVER_CORE,
                          PRODUCT_STORAGE_WORKGROUP_SERVER_CORE. Thanks to Petr Stejskal for prompting this update.
                          3. Added a IsHomeServerPremiumEditionInstalled method.
                          4. Fixed a bug handling PRODUCT_CLUSTER_SERVER_V ProductType
                          5. Fixed a bug handling PRODUCT_CLUSTER_SERVER ProductType
                          6. Fixed a typo in the name of IsSmallBusinessServerInstalled method
                          7. Added a IsWindowsCENET method.
                          8. Added a IsWindowsEmbeddedCompact method. 
                          9. The sample app has been updated to differentiate between the various marketing names for CE i.e. Windows CE,
                          Windows CE .NET and Windows Embedded Compact. The code has also been verified to operate correctly on Windows Embedded
                          Compact using a OSDesign project on Visual Studio 2008 Professional, Windows Embedded Compact, a sub project which
                          builds DtWinVer for x86 CEPC and a VMWare image of Compact 7 running the DtWinVer binaries. Thanks to Mike Ryan for 
                          prompting this update.
         PJN / 02-07-2011 1. Updated to support Windows 8 and Windows Server 2012. These are the names which DtWinVer currently uses to refer
                          to the next client and server versions of Windows respectively. At the moment there is no official details from 
                          Microsoft on what the product names will be, but DtWinVer will be updated following any announcements.
         PJN / 17-09-2011 1. Updated the code to work correctly against the Windows 8 Developer Preview and Windows 8 Server Developer 
                          Preview builds.
                          2. Updated code to refer to Windows codename "8" and Windows codename Server "8" instead of Windows 8 and Windows 
                          Server 2012.
         PJN / 28-04-2012 1. Updated copyright details.
                          2. My guess from last year was correct! Windows codename Server "8" will become Windows Server 2012. The code has been
                          updated to reflect this announcement from http://www.microsoft.com/en-us/news/Press/2012/Apr12/04-17MMSDay1PR.aspx.
                          3. Updated code to refer to Windows codename "8" as Windows 8 based on information from
                          http://windowsteamblog.com/windows/b/bloggingwindows/archive/2012/04/16/announcing-the-windows-8-editions.aspx.
                          4. Removed tests for Windows 8 (Home Basic), Windows 8 (Home Premium), Windows 8 (Ultimate) from code.
                          5. Added support for the following Product types: PRODUCT_PRERELEASE, PRODUCT_PRERELEASE_N, PRODUCT_PRERELEASE_ARM,
                          PRODUCT_PROFESSIONAL_PLUS, PRODUCT_ENTERPRISE_EVALUATION, PRODUCT_STANDARD_EVALUATION_SERVER, 
                          PRODUCT_DATACENTER_EVALUATION_SERVER, PRODUCT_ENTERPRISE_N_EVALUATION, PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER,
                          PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER, PRODUCT_MULTIPOINT_STANDARD_SERVER, PRODUCT_MULTIPOINT_PREMIUM_SERVER &
                          PRODUCT_THINPC, PRODUCT_EMBEDDEDINDUSTRY, PRODUCT_EMBEDDEDA, PRODUCT_EMBEDDEDINDUSTRYA, PRODUCT_EMBEDDEDAUTOMOTIVE,
                          PRODUCT_EMBEDDEDE, PRODUCT_EMBEDDEDINDUSTRYAE & PRODUCT_EMBEDDEDINDUSTRYE
                          6. Added support for Windows RT based on information from 
                          http://windowsteamblog.com/windows/b/bloggingwindows/archive/2012/04/16/announcing-the-windows-8-editions.aspx.
         PJN / 29/12-2012 1. Added support for the following product types: PRODUCT_EMBEDDED_AUTOMOTIVE, PRODUCT_EMBEDDED_INDUSTRY_A,
                          PRODUCT_EMBEDDED_A, PRODUCT_EMBEDDED_INDUSTRY, PRODUCT_EMBEDDED_E, PRODUCT_EMBEDDED_INDUSTRY_E,
                          PRODUCT_EMBEDDED_INDUSTRY_E, PRODUCT_EMBEDDED_INDUSTRY_A_E, PRODUCT_CORE_ARM, PRODUCT_CORE_ARM, PRODUCT_CORE_N,
                          PRODUCT_CORE_COUNTRYSPECIFIC, PRODUCT_CORE_SINGLELANGUAGE, PRODUCT_CORE & PRODUCT_PROFESSIONAL_WMC
                          2. Fixed a bug with the handling of the PRODUCT_THINPC #define value
                          3. Updated the code to compile cleanly on VC 2012
         PJN / 26-05-2013 1. Updated copyright details.
                          2. Now includes support for Windows 8.1 formerly known as Windows "Blue". I have also added code to support the
                          server version of "Blue" which for the moment I am calling Windows Server codename "Blue".
         PJN / 07-06-2013 1. Updated all code references of Windows Server codename "Blue" to Windows Server 2012 R2. 
         PJN / 21-07-2013 1. Fixed some /analyze warnings reported by VS 2013 Preview.
                          2. The code now correctly handles the case where Windows 8.1 and Windows 2012 R2 lies about the real windows 
                          version number. For details see http://msdn.microsoft.com/en-us/library/windows/desktop/dn302074(v=vs.85).aspx.
                          This change should also mean that correct OS details are returned when the code is run from a process in which
                          AppCompat version number shims are installed. Thanks to Christian Kaiser for reporting this issue.
                          3. All RegOpenKeyEx now requests the non virtualized key via the KEY_WOW64_64KEY value.
                          4. Reworked the WriteVer application to correctly compile under Unicode.
                          5. Win32s is now surfaced as a suite value.
                          6. Major rework and refactoring of all the DtWinver code base and test app. This rework should make it easy to 
                          update the code going forward.
                          7. The suite details are now only reported after the underlying OS details in the test app.
                          8. The sample app now uses a larger stack buffer for displaying the OS details. This avoids any potential 
                          stack overwrite issues when the number of suite mask values returned is large.
                          9. Fixed a bug in the generic thunking code path in GetProductInfo which resulted in the Win32 API of the same
                          name failing.
         PJN / 14-04-2014 1. Updated copyright details
                          2. Updated code to compile cleanly using VS 2013
                          3. Updated code to report failure when compiled under Windows RT i.e. when the WINAPI_FAMILY preprocessor 
                          value is anything other than WINAPI_FAMILY_DESKTOP_APP
                          4. Added support to detect Windows 8.1 / 2012 R2 Update
         PJN / 01-10-2014 1. Updated code to support Windows 10 Technical Preview and Windows Server 10 Technical Preview.
         PJN / 17-01-2015 1. Updated copyright details.
                          2. Fixed up the COSVersion::IsWindows2000AdvancedServer method to correctly distinguish between server and client OSes.
                          3. Fixed up the COSVersion::IsEnterpriseWindowsServer2003 method to correctly distinguish between server and client OSes.
                          4. Fixed up the COSVersion::IsEnterpriseWindowsServer2003R2 method to correctly distinguish between server and client OSes.
                          5. Fixed up the COSVersion::IsEnterpriseWindowsServer2008 method to correctly distinguish between server and client OSes.
                          6. Fixed up the COSVersion::IsEnterpriseWindowsServer2008R2 method to correctly distinguish between server and client OSes.
                          7. Fixed up the COSVersion::IsEnterpriseWindowsServer2012 method to correctly distinguish between server and client OSes.
                          8. Fixed up the COSVersion::IsEnterpriseWindowsServer2012R2 method to correctly distinguish between server and client OSes.
                          9. Fixed up the COSVersion::IsEnterpriseWindowsServer10 method to correctly distinguish between server and client OSes.
         PJN / 29-04-2015 1. Updated the code to correctly handle / identify Windows 8.1 with Bing (aka CoreConnected). This applies to the 
                          ProductInfo values: PRODUCT_CORE_ARM, PRODUCT_CORE_N, PRODUCT_CORE_COUNTRYSPECIFIC, PRODUCT_CORE_SINGLELANGUAGE & 
                          PRODUCT_CORE. Thanks to Petr Stejskal for reporting this issue. The suite value that is used to identify this is 
                          COSVERSION_SUITE2_CORECONNECTED and the method IsCoreConnected.
                          2. The ProductInfo value PRODUCT_PRERELEASE_ARM now correctly sets the underlying processor type to ARM
         PJN / 30-04-2015 1. It turns out that the new detection code to detect Windows 8.1 with Bing was flawed in the previous release as the 
                          PRODUCT_CORE_* ProductInfo values just means the standard / core version of Windows 8.1. The code has now been updated to 
                          use a registry search to correctly detect if Windows 8.1 with Bing is installed.
         PJN / 17-05-2015 1. Updated all code references of Windows Server 10 to Windows Server 2016.
                          2. Added support for the following product types: PRODUCT_PROFESSIONAL_EMBEDDED, PRODUCT_MOBILE_CORE, 
                          PRODUCT_EMBEDDED_INDUSTRY_EVAL, PRODUCT_EMBEDDED_INDUSTRY_E_EVAL, PRODUCT_EMBEDDED_EVAL, PRODUCT_EMBEDDED_E_EVAL,
                          PRODUCT_NANO_SERVER, PRODUCT_CLOUD_STORAGE_SERVER, PRODUCT_CORE_CONNECTED, PRODUCT_PROFESSIONAL_STUDENT,
                          PRODUCT_CORE_CONNECTED_N, PRODUCT_PROFESSIONAL_STUDENT_N, PRODUCT_CORE_CONNECTED_SINGLELANGUAGE,
                          PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC, PRODUCT_CONNECTED_CAR, PRODUCT_INDUSTRY_HANDHELD, PRODUCT_PPI_PRO,
                          PRODUCT_ARM64_SERVER, PRODUCT_EDUCATION, PRODUCT_EDUCATION_N, PRODUCT_IOTUAP, PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER,
                          PRODUCT_ENTERPRISE_S, PRODUCT_ENTERPRISE_S_N, PRODUCT_PROFESSIONAL_S, PRODUCT_PROFESSIONAL_S_N,
                          PRODUCT_ENTERPRISE_S_EVALUATION & PRODUCT_ENTERPRISE_S_N_EVALUATION.
                          3. Remove the word Installed from most of the methods.
                          4. The download now includes a Winver14.sln solution for Visual Studio 2015 RC. This now allows you to build an ARM 
                          binary of DtWinVer which you can deploy to Windows IoT Core on the Raspberry Pi 2. Also included is a precompiled
                          ARM binary of DtWinVer.
                          5. Updated the code to clean compile on VS 2015 RC
         PJN / 19-09-2015 1. Code now always tries to use "RTLGetVersion" to get the underlying OS details. Previously it would only do this if
                          the code detected that it was running on Windows 8.1 or greater. Also removed the GetRealVersionNumbersFromRegistry
                          method as this returns 6.3 on Windows 10 and as such is just unreliable. Please note that DtWinVer will still report
                          incorrect OS details if compatibility mode is enabled for your application. Thanks to Dave Silber for prompting this 
                          investigation.

Copyright (c) 1997 - 2015 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 
*/


/////////////////////////////////  Includes  //////////////////////////////////

#include "Dtwinver.h"
#pragma comment(lib, "version.lib")
/////////////////////////////////  Local function / variables /////////////////

#ifndef VER_NT_WORKSTATION
#define VER_NT_WORKSTATION 0x0000001
#endif

#ifndef VER_NT_DOMAIN_CONTROLLER
#define VER_NT_DOMAIN_CONTROLLER 0x0000002
#endif

#ifndef VER_NT_SERVER
#define VER_NT_SERVER 0x0000003
#endif

#ifndef PROCESSOR_ARCHITECTURE_SHX
#define PROCESSOR_ARCHITECTURE_SHX 4
#endif

#ifndef PROCESSOR_ARCHITECTURE_ARM
#define PROCESSOR_ARCHITECTURE_ARM 5
#endif

#ifndef PROCESSOR_ARCHITECTURE_IA64
#define PROCESSOR_ARCHITECTURE_IA64    6
#endif

#ifndef PROCESSOR_ARCHITECTURE_ALPHA64
#define PROCESSOR_ARCHITECTURE_ALPHA64 7
#endif

#ifndef PROCESSOR_ARCHITECTURE_MSIL
#define PROCESSOR_ARCHITECTURE_MSIL 8
#endif

#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64 9
#endif

#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 10
#endif

#ifndef SM_TABLETPC
#define SM_TABLETPC 86
#endif

#ifndef SM_MEDIACENTER
#define SM_MEDIACENTER 87
#endif

#ifndef SM_STARTER
#define SM_STARTER 88
#endif

#ifndef SM_SERVERR2
#define SM_SERVERR2 89
#endif

#ifndef VER_SUITE_SMALLBUSINESS
#define VER_SUITE_SMALLBUSINESS 0x00000001
#endif

#ifndef VER_SUITE_ENTERPRISE
#define VER_SUITE_ENTERPRISE 0x00000002
#endif

#ifndef VER_SUITE_BACKOFFICE
#define VER_SUITE_BACKOFFICE 0x00000004
#endif

#ifndef VER_SUITE_TERMINAL
#define VER_SUITE_TERMINAL 0x00000010
#endif

#ifndef VER_SUITE_EMBEDDEDNT
#define VER_SUITE_EMBEDDEDNT 0x00000040
#endif

#ifndef VER_SUITE_DATACENTER
#define VER_SUITE_DATACENTER 0x00000080
#endif

#ifndef VER_SUITE_SINGLEUSERTS
#define VER_SUITE_SINGLEUSERTS 0x00000100
#endif

#ifndef VER_SUITE_PERSONAL
#define VER_SUITE_PERSONAL 0x00000200
#endif

#ifndef VER_SUITE_BLADE
#define VER_SUITE_BLADE 0x00000400
#endif

#ifndef VER_SUITE_SECURITY_APPLIANCE
#define VER_SUITE_SECURITY_APPLIANCE 0x00001000
#endif

#ifndef VER_SUITE_STORAGE_SERVER
#define VER_SUITE_STORAGE_SERVER 0x00002000
#endif

#ifndef VER_SUITE_COMPUTE_SERVER
#define VER_SUITE_COMPUTE_SERVER 0x00004000
#endif

#ifndef VER_SUITE_WH_SERVER
#define VER_SUITE_WH_SERVER 0x00008000 
#endif

//Flags returned from GetProductInfo
#ifndef PRODUCT_ULTIMATE
#define PRODUCT_ULTIMATE 0x00000001
#endif

#ifndef PRODUCT_HOME_BASIC
#define PRODUCT_HOME_BASIC 0x00000002
#endif

#ifndef PRODUCT_HOME_PREMIUM
#define PRODUCT_HOME_PREMIUM 0x00000003
#endif 

#ifndef PRODUCT_ENTERPRISE
#define PRODUCT_ENTERPRISE 0x00000004
#endif 

#ifndef PRODUCT_HOME_BASIC_N
#define PRODUCT_HOME_BASIC_N 0x00000005
#endif

#ifndef PRODUCT_BUSINESS
#define PRODUCT_BUSINESS 0x00000006
#endif

#ifndef PRODUCT_STANDARD_SERVER
#define PRODUCT_STANDARD_SERVER 0x00000007
#endif

#ifndef PRODUCT_DATACENTER_SERVER
#define PRODUCT_DATACENTER_SERVER 0x00000008
#endif

#ifndef PRODUCT_SMALLBUSINESS_SERVER
#define PRODUCT_SMALLBUSINESS_SERVER 0x00000009
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER
#define PRODUCT_ENTERPRISE_SERVER 0x0000000A
#endif

#ifndef PRODUCT_STARTER
#define PRODUCT_STARTER 0x0000000B
#endif

#ifndef PRODUCT_DATACENTER_SERVER_CORE
#define PRODUCT_DATACENTER_SERVER_CORE 0x0000000C
#endif

#ifndef PRODUCT_STANDARD_SERVER_CORE
#define PRODUCT_STANDARD_SERVER_CORE 0x0000000D
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER_CORE
#define PRODUCT_ENTERPRISE_SERVER_CORE 0x0000000E
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER_IA64
#define PRODUCT_ENTERPRISE_SERVER_IA64 0x0000000F
#endif

#ifndef PRODUCT_BUSINESS_N
#define PRODUCT_BUSINESS_N 0x00000010
#endif

#ifndef PRODUCT_WEB_SERVER
#define PRODUCT_WEB_SERVER 0x00000011
#endif

#ifndef PRODUCT_CLUSTER_SERVER
#define PRODUCT_CLUSTER_SERVER 0x00000012
#endif

#ifndef PRODUCT_HOME_SERVER
#define PRODUCT_HOME_SERVER 0x00000013
#endif

#ifndef PRODUCT_STORAGE_EXPRESS_SERVER
#define PRODUCT_STORAGE_EXPRESS_SERVER 0x00000014
#endif

#ifndef PRODUCT_STORAGE_STANDARD_SERVER
#define PRODUCT_STORAGE_STANDARD_SERVER 0x00000015
#endif

#ifndef PRODUCT_STORAGE_WORKGROUP_SERVER
#define PRODUCT_STORAGE_WORKGROUP_SERVER 0x00000016
#endif

#ifndef PRODUCT_STORAGE_ENTERPRISE_SERVER
#define PRODUCT_STORAGE_ENTERPRISE_SERVER 0x00000017
#endif

#ifndef PRODUCT_SERVER_FOR_SMALLBUSINESS
#define PRODUCT_SERVER_FOR_SMALLBUSINESS 0x00000018
#endif

#ifndef PRODUCT_SMALLBUSINESS_SERVER_PREMIUM
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM 0x00000019
#endif

#ifndef PRODUCT_HOME_PREMIUM_N
#define PRODUCT_HOME_PREMIUM_N 0x0000001A
#endif

#ifndef PRODUCT_ENTERPRISE_N
#define PRODUCT_ENTERPRISE_N 0x0000001B
#endif

#ifndef PRODUCT_ULTIMATE_N
#define PRODUCT_ULTIMATE_N 0x0000001C
#endif

#ifndef PRODUCT_WEB_SERVER_CORE
#define PRODUCT_WEB_SERVER_CORE 0x0000001D
#endif

#ifndef PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT
#define PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT 0x0000001E
#endif

#ifndef PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY
#define PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY 0x0000001F
#endif

#ifndef PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING
#define PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING 0x00000020
#endif

#ifndef PRODUCT_SERVER_FOUNDATION
#define PRODUCT_SERVER_FOUNDATION 0x00000021
#endif

#ifndef PRODUCT_HOME_PREMIUM_SERVER
#define PRODUCT_HOME_PREMIUM_SERVER 0x00000022
#endif

#ifndef PRODUCT_SERVER_FOR_SMALLBUSINESS_V
#define PRODUCT_SERVER_FOR_SMALLBUSINESS_V 0x00000023
#endif

#ifndef PRODUCT_STANDARD_SERVER_V
#define PRODUCT_STANDARD_SERVER_V 0x00000024 
#endif

#ifndef PRODUCT_DATACENTER_SERVER_V
#define PRODUCT_DATACENTER_SERVER_V 0x00000025
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER_V
#define PRODUCT_ENTERPRISE_SERVER_V 0x00000026
#endif 

#ifndef PRODUCT_DATACENTER_SERVER_CORE_V
#define PRODUCT_DATACENTER_SERVER_CORE_V 0x00000027
#endif

#ifndef PRODUCT_STANDARD_SERVER_CORE_V
#define PRODUCT_STANDARD_SERVER_CORE_V 0x00000028
#endif

#ifndef PRODUCT_ENTERPRISE_SERVER_CORE_V
#define PRODUCT_ENTERPRISE_SERVER_CORE_V 0x00000029
#endif

#ifndef PRODUCT_HYPERV
#define PRODUCT_HYPERV 0x0000002A
#endif

#ifndef PRODUCT_STORAGE_EXPRESS_SERVER_CORE
#define PRODUCT_STORAGE_EXPRESS_SERVER_CORE 0x0000002B
#endif

#ifndef PRODUCT_STORAGE_STANDARD_SERVER_CORE 
#define PRODUCT_STORAGE_STANDARD_SERVER_CORE 0x0000002C
#endif 

#ifndef PRODUCT_STORAGE_WORKGROUP_SERVER_CORE
#define PRODUCT_STORAGE_WORKGROUP_SERVER_CORE 0x0000002D
#endif

#ifndef PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE
#define PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE 0x0000002E
#endif

#ifndef PRODUCT_STARTER_N
#define PRODUCT_STARTER_N 0x0000002F
#endif

#ifndef PRODUCT_PROFESSIONAL
#define PRODUCT_PROFESSIONAL 0x00000030
#endif

#ifndef PRODUCT_PROFESSIONAL_N
#define PRODUCT_PROFESSIONAL_N 0x00000031
#endif

#ifndef PRODUCT_SB_SOLUTION_SERVER
#define PRODUCT_SB_SOLUTION_SERVER 0x00000032
#endif

#ifndef PRODUCT_SERVER_FOR_SB_SOLUTIONS
#define PRODUCT_SERVER_FOR_SB_SOLUTIONS 0x00000033
#endif

#ifndef PRODUCT_STANDARD_SERVER_SOLUTIONS
#define PRODUCT_STANDARD_SERVER_SOLUTIONS 0x00000034
#endif

#ifndef PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE
#define PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE 0x00000035
#endif 

#ifndef PRODUCT_SB_SOLUTION_SERVER_EM
#define PRODUCT_SB_SOLUTION_SERVER_EM 0x00000036
#endif

#ifndef PRODUCT_SERVER_FOR_SB_SOLUTIONS_EM
#define PRODUCT_SERVER_FOR_SB_SOLUTIONS_EM 0x00000037
#endif

#ifndef PRODUCT_SOLUTION_EMBEDDEDSERVER
#define PRODUCT_SOLUTION_EMBEDDEDSERVER 0x00000038
#endif

#ifndef PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE
#define PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE 0x00000039
#endif

#ifndef PRODUCT_PROFESSIONAL_EMBEDDED
#define PRODUCT_PROFESSIONAL_EMBEDDED 0x0000003A
#endif

#ifndef PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT
#define PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT 0x0000003B
#endif

#ifndef PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL
#define PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL 0x0000003C
#endif

#ifndef PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC
#define PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC 0x0000003D
#endif

#ifndef PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC
#define PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC 0x0000003E
#endif

#ifndef PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE
#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE 0x0000003F
#endif

#ifndef PRODUCT_CLUSTER_SERVER_V
#define PRODUCT_CLUSTER_SERVER_V 0x00000040
#endif

#ifndef PRODUCT_EMBEDDED
#define PRODUCT_EMBEDDED 0x00000041
#endif

#ifndef PRODUCT_STARTER_E
#define PRODUCT_STARTER_E 0x00000042
#endif

#ifndef PRODUCT_HOME_BASIC_E
#define PRODUCT_HOME_BASIC_E 0x00000043
#endif

#ifndef PRODUCT_HOME_PREMIUM_E
#define PRODUCT_HOME_PREMIUM_E 0x00000044
#endif

#ifndef PRODUCT_PROFESSIONAL_E
#define PRODUCT_PROFESSIONAL_E 0x00000045
#endif

#ifndef PRODUCT_ENTERPRISE_E
#define PRODUCT_ENTERPRISE_E 0x00000046
#endif

#ifndef PRODUCT_ULTIMATE_E
#define PRODUCT_ULTIMATE_E 0x00000047
#endif

#ifndef PRODUCT_ENTERPRISE_EVALUATION
#define PRODUCT_ENTERPRISE_EVALUATION 0x00000048
#endif

#ifndef PRODUCT_PRERELEASE
#define PRODUCT_PRERELEASE 0x0000004A
#endif

#ifndef PRODUCT_MULTIPOINT_STANDARD_SERVER
#define PRODUCT_MULTIPOINT_STANDARD_SERVER 0x0000004C
#endif

#ifndef PRODUCT_MULTIPOINT_PREMIUM_SERVER
#define PRODUCT_MULTIPOINT_PREMIUM_SERVER 0x0000004D
#endif

#ifndef PRODUCT_STANDARD_EVALUATION_SERVER
#define PRODUCT_STANDARD_EVALUATION_SERVER 0x0000004F
#endif

#ifndef PRODUCT_DATACENTER_EVALUATION_SERVER
#define PRODUCT_DATACENTER_EVALUATION_SERVER 0x00000050
#endif

#ifndef PRODUCT_PRERELEASE_ARM
#define PRODUCT_PRERELEASE_ARM 0x00000051
#endif

#ifndef PRODUCT_PRERELEASE_N
#define PRODUCT_PRERELEASE_N 0x00000052
#endif

#ifndef PRODUCT_ENTERPRISE_N_EVALUATION
#define PRODUCT_ENTERPRISE_N_EVALUATION 0x00000054
#endif

#ifndef PRODUCT_EMBEDDED_AUTOMOTIVE
#define PRODUCT_EMBEDDED_AUTOMOTIVE 0x00000055 
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY_A
#define PRODUCT_EMBEDDED_INDUSTRY_A 0x00000056 
#endif

#ifndef PRODUCT_THINPC
#define PRODUCT_THINPC 0x00000057 
#endif

#ifndef PRODUCT_EMBEDDED_A
#define PRODUCT_EMBEDDED_A 0x00000058
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY
#define PRODUCT_EMBEDDED_INDUSTRY 0x00000059
#endif

#ifndef PRODUCT_EMBEDDED_E
#define PRODUCT_EMBEDDED_E 0x0000005A
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY_E
#define PRODUCT_EMBEDDED_INDUSTRY_E 0x0000005B 
#endif

#ifndef PRODUCT_EMBEDDED_INDUSTRY_A_E
#define PRODUCT_EMBEDDED_INDUSTRY_A_E 0x0000005C
#endif

#ifndef PRODUCT_PROFESSIONAL_PLUS
#define PRODUCT_PROFESSIONAL_PLUS 0x0000005D 
#endif

#ifndef PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER
#define PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER 0x0000005F 
#endif

#ifndef PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER
#define PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER 0x00000060 
#endif

#ifndef PRODUCT_CORE_ARM
#define PRODUCT_CORE_ARM 0x00000061
#endif

#ifndef PRODUCT_CORE_N
#define PRODUCT_CORE_N 0x00000062
#endif

#ifndef PRODUCT_CORE_COUNTRYSPECIFIC
#define PRODUCT_CORE_COUNTRYSPECIFIC 0x00000063
#endif

#ifndef PRODUCT_CORE_SINGLELANGUAGE
#define PRODUCT_CORE_SINGLELANGUAGE 0x00000064
#endif

#ifndef PRODUCT_CORE
#define PRODUCT_CORE 0x00000065
#endif

#ifndef PRODUCT_PROFESSIONAL_WMC
#define PRODUCT_PROFESSIONAL_WMC 0x00000067
#endif

#ifndef PRODUCT_MOBILE_CORE
#define PRODUCT_MOBILE_CORE 0x00000068
#endif 

#ifndef PRODUCT_EMBEDDED_INDUSTRY_EVAL
#define PRODUCT_EMBEDDED_INDUSTRY_EVAL 0x00000069
#endif 

#ifndef PRODUCT_EMBEDDED_INDUSTRY_E_EVAL
#define PRODUCT_EMBEDDED_INDUSTRY_E_EVAL 0x0000006A
#endif 

#ifndef PRODUCT_EMBEDDED_EVAL
#define PRODUCT_EMBEDDED_EVAL 0x0000006B
#endif

#ifndef PRODUCT_EMBEDDED_E_EVAL
#define PRODUCT_EMBEDDED_E_EVAL 0x0000006C
#endif

#ifndef PRODUCT_NANO_SERVER
#define PRODUCT_NANO_SERVER 0x0000006D
#endif 

#ifndef PRODUCT_CLOUD_STORAGE_SERVER
#define PRODUCT_CLOUD_STORAGE_SERVER 0x0000006E
#endif

#ifndef PRODUCT_CORE_CONNECTED
#define PRODUCT_CORE_CONNECTED 0x0000006F
#endif

#ifndef PRODUCT_PROFESSIONAL_STUDENT
#define PRODUCT_PROFESSIONAL_STUDENT 0x00000070
#endif

#ifndef PRODUCT_CORE_CONNECTED_N
#define PRODUCT_CORE_CONNECTED_N 0x00000071
#endif

#ifndef PRODUCT_PROFESSIONAL_STUDENT_N
#define PRODUCT_PROFESSIONAL_STUDENT_N 0x00000072
#endif

#ifndef PRODUCT_CORE_CONNECTED_SINGLELANGUAGE
#define PRODUCT_CORE_CONNECTED_SINGLELANGUAGE 0x00000073
#endif

#ifndef PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC
#define PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC 0x00000074
#endif

#ifndef PRODUCT_CONNECTED_CAR
#define PRODUCT_CONNECTED_CAR 0x00000075
#endif

#ifndef PRODUCT_INDUSTRY_HANDHELD
#define PRODUCT_INDUSTRY_HANDHELD 0x00000076
#endif

#ifndef PRODUCT_PPI_PRO
#define PRODUCT_PPI_PRO 0x00000077
#endif

#ifndef PRODUCT_ARM64_SERVER
#define PRODUCT_ARM64_SERVER 0x00000078
#endif

#ifndef PRODUCT_EDUCATION
#define PRODUCT_EDUCATION 0x00000079
#endif

#ifndef PRODUCT_EDUCATION_N
#define PRODUCT_EDUCATION_N 0x0000007A
#endif

#ifndef PRODUCT_IOTUAP
#define PRODUCT_IOTUAP 0x0000007B
#endif

#ifndef PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER
#define PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER 0x0000007C
#endif

#ifndef PRODUCT_ENTERPRISE_S
#define PRODUCT_ENTERPRISE_S 0x0000007D
#endif

#ifndef PRODUCT_ENTERPRISE_S_N
#define PRODUCT_ENTERPRISE_S_N 0x0000007E
#endif

#ifndef PRODUCT_PROFESSIONAL_S
#define PRODUCT_PROFESSIONAL_S 0x0000007F
#endif

#ifndef PRODUCT_PROFESSIONAL_S_N
#define PRODUCT_PROFESSIONAL_S_N 0x00000080
#endif

#ifndef PRODUCT_ENTERPRISE_S_EVALUATION
#define PRODUCT_ENTERPRISE_S_EVALUATION 0x00000081
#endif

#ifndef PRODUCT_ENTERPRISE_S_N_EVALUATION
#define PRODUCT_ENTERPRISE_S_N_EVALUATION 0x00000082
#endif


#ifndef VER_PLATFORM_WIN32_CE
#define VER_PLATFORM_WIN32_CE 3
#endif

#ifndef SPI_GETPLATFORMTYPE
#define SPI_GETPLATFORMTYPE 257
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif


#if defined(COSVERSION_DOS)
  WORD WinVer;
  BYTE bRunningWindows;
#endif //#if defined(COSVERSION_DOS)


////////////////////////////////// Implementation /////////////////////////////

COSVersion::COSVersion()
{
#if defined(COSVERSION_WIN16)
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
  m_lpfnGetProductInfo = 0;

  //Get Kernel dll handle
  HMODULE hKernel = GetModuleHandle("KERNEL");
  if (hKernel != NULL)
  {
    //Dynamically link to the functions we want
    m_lpfnLoadLibraryEx32W  = (lpfnLoadLibraryEx32W)  GetProcAddress(hKernel, "LoadLibraryEx32W");
    m_lpfnFreeLibrary32W    = (lpfnFreeLibrary32W)    GetProcAddress(hKernel, "FreeLibrary32W");
    m_lpfnGetProcAddress32W = (lpfnGetProcAddress32W) GetProcAddress(hKernel, "GetProcAddress32W");
    m_lpfnCallProcEx32W     = (lpfnCallProcEx32W)     GetProcAddress(hKernel, "_CallProcEx32W");
    if (m_lpfnLoadLibraryEx32W && m_lpfnFreeLibrary32W)
    {
      m_hAdvApi32 = m_lpfnLoadLibraryEx32W("ADVAPI32.DLL", NULL, 0);
      if (m_hAdvApi32 != NULL)
        m_lpfnRegQueryValueExA = m_lpfnGetProcAddress32W(m_hAdvApi32, "RegQueryValueExA"); 
      m_hKernel32 = m_lpfnLoadLibraryEx32W("KERNEL32.DLL", NULL, 0);
      if (m_hKernel32 != NULL)
      {                                                               
        m_lpfnGetVersionExA = m_lpfnGetProcAddress32W(m_hKernel32, "GetVersionExA");
        m_lpfnGetVersion = m_lpfnGetProcAddress32W(m_hKernel32, "GetVersion");
        m_lpfnGetSystemInfo = m_lpfnGetProcAddress32W(m_hKernel32, "GetSystemInfo");
        m_lpfnGetNativeSystemInfo = m_lpfnGetProcAddress32W(m_hKernel32, "GetNativeSystemInfo");
        m_lpfnGetProductInfo = m_lpfnGetProcAddress32W(m_hKernel32, "GetProductInfo");
      }
    }
  }                         
#endif //#if defined(COSVERSION_WIN16)
}

COSVersion::~COSVersion()
{
#if defined(COSVERSION_WIN16)
  if (m_lpfnFreeLibrary32W != NULL)
  {           
    if (m_hAdvApi32 != NULL)
      m_lpfnFreeLibrary32W(m_hAdvApi32);
    if (m_hKernel32 != NULL)
      m_lpfnFreeLibrary32W(m_hKernel32);
  }
#endif //#if defined(COSVERSION_WIN16)
}

#if defined(COSVERSION_WIN16)
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
      //a multinet driver is installed
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
  if (hUserInst != NULL)
    FreeLibrary(hUserInst);
    
  return rVal;
}

LONG COSVersion::RegQueryValueEx(HKEY32 hKey, LPSTR lpszValueName, LPDWORD lpdwReserved, LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData)
{                                             
  //Assume the worst
  DWORD dwResult = ERROR_CALL_NOT_IMPLEMENTED;

  if (m_lpfnRegQueryValueExA)
    dwResult = m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 6, 0x3E, m_lpfnRegQueryValueExA, hKey, lpszValueName, lpdwReserved, lpdwType, lpbData, lpcbData);

  return dwResult;
}                 
#endif //#if defined(COSVERSION_WIN16)

COSVersion::PROCESSOR_TYPE COSVersion::MapProcessorArchitecture(WORD wProcessorArchitecture)
{
  //What will be the return value from this method
  PROCESSOR_TYPE ProcessorType = UNKNOWN_PROCESSOR;

  //Map from the SYTEM_INFO wProcessorArchitecture defines to our equivalents
  switch (wProcessorArchitecture)
  {
    case PROCESSOR_ARCHITECTURE_INTEL: //deliberate fallthrough
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
    {
      ProcessorType = IA32_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_MSIL:
    {
      ProcessorType = MSIL_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_MIPS:
    {
      ProcessorType = MIPS_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ARM:
    {
      ProcessorType = ARM_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_SHX:
    {
      ProcessorType = SHX_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ALPHA:
    {
      ProcessorType = ALPHA_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_ALPHA64:
    {
      ProcessorType = ALPHA64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_PPC:
    {
      ProcessorType = PPC_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_IA64:
    {
      ProcessorType = IA64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_AMD64:
    {
      ProcessorType = AMD64_PROCESSOR;
      break;
    }
    case PROCESSOR_ARCHITECTURE_UNKNOWN: //Deliberate fallthrough
    default:
    {
      ProcessorType = UNKNOWN_PROCESSOR;
      break;
    }
  }

  return ProcessorType;
}
                  
#if defined(COSVERSION_DOS)
BOOL COSVersion::GetProcessorType(LPOS_VERSION_INFO /*lpVersionInformation*/)
#else
BOOL COSVersion::GetProcessorType(LPOS_VERSION_INFO lpVersionInformation)
#endif //#if defined(COSVERSION_DOS)
{             
  //What will be the return value from this method (assume the worst)
  BOOL bSuccess = FALSE;

  SYSTEM_INFO EmulatedSI;
  EmulatedSI.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
  SYSTEM_INFO UnderlyingSI;
  UnderlyingSI.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
#if !defined(COSVERSION_CE)
#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)) || (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PC_APP))
  GetNativeSystemInfo(&UnderlyingSI);
  memcpy(&EmulatedSI, &UnderlyingSI, sizeof(SYSTEM_INFO));

  //Map from the SYTEM_INFO wProcessorArchitecture defines to our equivalents
  lpVersionInformation->UnderlyingProcessorType = MapProcessorArchitecture(UnderlyingSI.wProcessorArchitecture);
  lpVersionInformation->EmulatedProcessorType = MapProcessorArchitecture(EmulatedSI.wProcessorArchitecture);
#else
  //Get Kernel handle
  HMODULE hKernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
  if (hKernel32 != NULL)
  {
    bSuccess = TRUE;

    lpfnGetNativeSystemInfo pGetNativeSystemInfo = (lpfnGetNativeSystemInfo) GetProcAddress(hKernel32, "GetNativeSystemInfo"); 
    if (pGetNativeSystemInfo)
    {
      pGetNativeSystemInfo(&UnderlyingSI);
      GetSystemInfo(&EmulatedSI);
    }
    else
    {
      GetSystemInfo(&UnderlyingSI);
      memcpy(&EmulatedSI, &UnderlyingSI, sizeof(SYSTEM_INFO));
    }

    //Map from the SYTEM_INFO wProcessorArchitecture defines to our equivalents
    lpVersionInformation->UnderlyingProcessorType = MapProcessorArchitecture(UnderlyingSI.wProcessorArchitecture);
    lpVersionInformation->EmulatedProcessorType = MapProcessorArchitecture(EmulatedSI.wProcessorArchitecture);
  }
#endif //#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)) || (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PC_APP))
#endif //#if !defined(COSVERSION_CE)
#elif defined(COSVERSION_WIN16)
  bSuccess = TRUE;
                    
  //Set the processor types to default values on the Win16 code path	                    
  lpVersionInformation->UnderlyingProcessorType = IA32_PROCESSOR;
  lpVersionInformation->EmulatedProcessorType = IA32_PROCESSOR;
  
  if (m_lpfnGetNativeSystemInfo)
  {
    m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 1, 0x01, m_lpfnGetNativeSystemInfo, &UnderlyingSI, 0);
  
    if (m_lpfnGetSystemInfo)
      m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 1, 0x01, m_lpfnGetSystemInfo, &EmulatedSI, 0);
  }
  else
  {
    if (m_lpfnGetSystemInfo)
      m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 1, 0x01, m_lpfnGetSystemInfo, &EmulatedSI, 0);

    memcpy(&EmulatedSI, &UnderlyingSI, sizeof(SYSTEM_INFO));
  }
  
  //Map from the SYTEM_INFO wProcessorArchitecture defines to our equivalents
  lpVersionInformation->UnderlyingProcessorType = MapProcessorArchitecture(UnderlyingSI.wProcessorArchitecture);
  lpVersionInformation->EmulatedProcessorType = MapProcessorArchitecture(EmulatedSI.wProcessorArchitecture);
#endif //#if defined(COSVERSION_WIN16)

  return bSuccess;
}

DWORD COSVersion::MapWin32SuiteMask(WORD wSuiteMask)
{
  //What will be the return value from this method
  DWORD dwMappedSuiteMask = 0;

  //Explicitly map the Win32 wSuiteMask to our own values
  if (wSuiteMask & VER_SUITE_SMALLBUSINESS)
    dwMappedSuiteMask |= COSVERSION_SUITE_SMALLBUSINESS;
  if (wSuiteMask & VER_SUITE_ENTERPRISE)
    dwMappedSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
  if (wSuiteMask & VER_SUITE_TERMINAL)
    dwMappedSuiteMask |= COSVERSION_SUITE_TERMINAL;
  if (wSuiteMask & VER_SUITE_DATACENTER)
    dwMappedSuiteMask |= COSVERSION_SUITE_DATACENTER;
  if (wSuiteMask & VER_SUITE_PERSONAL)
    dwMappedSuiteMask |= COSVERSION_SUITE_PERSONAL;
  if (wSuiteMask & VER_SUITE_BLADE)
    dwMappedSuiteMask |= COSVERSION_SUITE_WEBEDITION;
  if (wSuiteMask & VER_SUITE_EMBEDDEDNT)
    dwMappedSuiteMask |= COSVERSION_SUITE_EMBEDDED;
  if (wSuiteMask & VER_SUITE_SINGLEUSERTS)
    dwMappedSuiteMask |= COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL;
  if (wSuiteMask & VER_SUITE_COMPUTE_SERVER)
    dwMappedSuiteMask |= COSVERSION_SUITE_COMPUTE_SERVER;
  if (wSuiteMask & VER_SUITE_STORAGE_SERVER)
    dwMappedSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
  if (wSuiteMask & VER_SUITE_SECURITY_APPLIANCE)
    dwMappedSuiteMask |= COSVERSION_SUITE_SECURITY_APPLIANCE;
  if (wSuiteMask & VER_SUITE_BACKOFFICE)
    dwMappedSuiteMask |= COSVERSION_SUITE_BACKOFFICE;
  if (wSuiteMask & VER_SUITE_WH_SERVER)
    dwMappedSuiteMask |= COSVERSION_SUITE_HOME_SERVER;

  return dwMappedSuiteMask;
}

COSVersion::OS_TYPE COSVersion::MapWin32ProductType(WORD wProductType)
{
  //what will be the return value from this method
  OS_TYPE ostype = UnknownOSType;

  switch (wProductType)
  {
    case VER_NT_WORKSTATION:
    {
      ostype = Workstation;
      break;
    }
    case VER_NT_DOMAIN_CONTROLLER:
    {
      ostype = DomainController;
      break;
    }
    case VER_NT_SERVER:
    {
      ostype = Server;
      break;
    }
    default:
    {
      break;
    }
  }

  return ostype;
}

COSVersion::OS_PLATFORM COSVersion::MapWin32PlatformId(DWORD dwPlatformId)
{
  //What will be the return value from this method
  OS_PLATFORM osPlatform = UnknownOSPlatform;

  //Explicitly map the win32 dwPlatformId to our own values 
  switch (dwPlatformId)
  {
    case VER_PLATFORM_WIN32_WINDOWS:
    {
      osPlatform = Windows9x;
      break;
    }
    case VER_PLATFORM_WIN32_NT:
    {
      osPlatform = WindowsNT;
      break;
    }
    case VER_PLATFORM_WIN32_CE:
    {
      osPlatform = WindowsCE;
      break;
    }
    default:
    {
      break;
    }
  }

  return osPlatform;
}


BOOL COSVersion::GetVersion(LPOS_VERSION_INFO lpVersionInformation)
{
  //Zero out everything in the structure
  memset(lpVersionInformation, 0, sizeof(OS_VERSION_INFO));
                
#if defined(COSVERSION_CE)
  OSVERSIONINFO osvi;
  memset(&osvi, 0, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (!GetVersionEx(&osvi))
    return FALSE;

  //Basic OS info
  lpVersionInformation->dwEmulatedMajorVersion = osvi.dwMajorVersion; 
  lpVersionInformation->dwEmulatedMinorVersion = osvi.dwMinorVersion; 
  lpVersionInformation->dwEmulatedBuildNumber = LOWORD(osvi.dwBuildNumber); //ignore HIWORD
  lpVersionInformation->EmulatedPlatform = WindowsCE;
  _tcscpy(lpVersionInformation->szEmulatedCSDVersion, osvi.szCSDVersion);

  //OEM Info
  SystemParametersInfo(SPI_GETOEMINFO, 256, lpVersionInformation->szOEMInfo, 0);

  //Platform Type
  SystemParametersInfo(SPI_GETPLATFORMTYPE, 256, lpVersionInformation->szPlatformType, 0);

  //Always set the OSType to Workstation on CE. The variable itself does not make 
  //much sense on CE
  lpVersionInformation->OSType = Workstation;

  //The underlying values will be the same as the emulated values
  lpVersionInformation->dwUnderlyingMajorVersion = lpVersionInformation->dwEmulatedMajorVersion; 
  lpVersionInformation->dwUnderlyingMinorVersion = lpVersionInformation->dwEmulatedMinorVersion; 
  lpVersionInformation->dwUnderlyingBuildNumber = lpVersionInformation->dwEmulatedBuildNumber; 
  lpVersionInformation->UnderlyingPlatform = lpVersionInformation->EmulatedPlatform; 
  _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, lpVersionInformation->szEmulatedCSDVersion); 

#elif defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  #if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
    //Try to use the GetVersionEx API with a OSVERSIONINFOEX struct or as a fallback a OSVERSIONINFO struct
    OSVERSIONINFOEX osviex;
    memset(&osviex, 0, sizeof(OSVERSIONINFOEX));
    osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OSVERSIONINFO osvi;
    memset(&osvi, 0, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    BOOL bGotOSviEx = GetVersionEx(&osviex);
    BOOL bGotOSvi = FALSE;
    if (!bGotOSviEx)
      bGotOSvi = GetVersionEx(&osvi);

    if (bGotOSviEx)
    {
      //Explicitly map the Win32 wSuiteMask to our own values
      lpVersionInformation->dwSuiteMask = MapWin32SuiteMask(osviex.wSuiteMask);

      //Explicitly map the Win32 wProductType to our own values
      lpVersionInformation->OSType = MapWin32ProductType(osviex.wProductType);

      //Call to the helper method to do the heavy lifting
      _GetVersion(osviex.dwMajorVersion, osviex.dwMinorVersion, osviex.dwBuildNumber, osviex.szCSDVersion, osviex.wServicePackMajor, 
                  osviex.wServicePackMinor, osviex.dwPlatformId, TRUE, lpVersionInformation);

      //Determine if we are using the "Standard" version of the server  
      if ((lpVersionInformation->OSType != Workstation) && ((osviex.wSuiteMask & COSVERSION_SUITE_ENTERPRISE) == 0) &&
          ((osviex.wSuiteMask & COSVERSION_SUITE_DATACENTER) == 0) && ((osviex.wSuiteMask & COSVERSION_SUITE_WEBEDITION) == 0))
        lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
    }
    else if (bGotOSvi)
    {
      //Determine the Service pack details prior to calling _GetVersion
      WORD wServicePackMajor = 0;
      WORD wServicePackMinor = 0;
      if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        wServicePackMajor = GetNTServicePackFromCSDString(osvi.szCSDVersion);
      else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
      {
        //Form the Windows 9x Service pack info
        if (IsWindows95SP1(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
          wServicePackMajor = 1;
        else if (IsWindows95B(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
          wServicePackMajor = 2;
        else if (IsWindows95C(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
        {
          wServicePackMajor = 2;
          wServicePackMinor = 5;
        }
        else if (IsWindows98SP1(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
          wServicePackMajor = 1;
      }

      //Call to the helper method to do the heavy lifting
      _GetVersion(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.szCSDVersion, wServicePackMajor,
                  wServicePackMinor, osvi.dwPlatformId, FALSE, lpVersionInformation);

      if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
      {
        //Get the Product Suites installed
        GetProductSuiteDetailsFromRegistry(lpVersionInformation);

        //Determine if we are using the "Standard" version of the server  
        if ((lpVersionInformation->OSType != Workstation) && ((osviex.wSuiteMask & COSVERSION_SUITE_ENTERPRISE) == 0) &&
            ((osviex.wSuiteMask & COSVERSION_SUITE_DATACENTER) == 0) && ((osviex.wSuiteMask & COSVERSION_SUITE_WEBEDITION) == 0))
          lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;

        //Get the terminal services details
        GetTerminalServicesRemoteAdminModeDetailsFromRegistry(lpVersionInformation);
      }
    }
    else
    {
      //Fall back to the GetVersion API
      DWORD dwVersion = 0;
      if (!GetVersion(dwVersion))
        return FALSE;

      //Determine the build number prior to calling _GetVersion
      DWORD dwBuildNumber = 0;
      GetNTCurrentBuildFromRegistry(dwBuildNumber);

      //Call to the helper method to do the heavy lifting (Note that the PlatformId can only be VER_PLATFORM_WIN32_NT because the check for GetVersionEx 
      //above can only fail on early versions of Windows NT)
      _GetVersion((DWORD)(LOBYTE(LOWORD(dwVersion))), (DWORD)(HIBYTE(LOWORD(dwVersion))), dwBuildNumber, _T(""), 0, 0, VER_PLATFORM_WIN32_NT, FALSE, lpVersionInformation);

      //Get the Product Suites installed
      GetProductSuiteDetailsFromRegistry(lpVersionInformation);

      //Determine if we are using the "Standard" version of the server  
      if ((lpVersionInformation->OSType != Workstation) && ((osviex.wSuiteMask & COSVERSION_SUITE_ENTERPRISE) == 0) &&
          ((osviex.wSuiteMask & COSVERSION_SUITE_DATACENTER) == 0) && ((osviex.wSuiteMask & COSVERSION_SUITE_WEBEDITION) == 0))
        lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;

      //Get the terminal services details
      GetTerminalServicesRemoteAdminModeDetailsFromRegistry(lpVersionInformation);
    }
  #else //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)

    #error "DtWinVer is not supported when the WINAPI_FAMILY preprocessor value is anything other than WINAPI_FAMILY_DESKTOP_APP"

  #endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#else //We must be runing on an emulated or real version of Win16 or DOS
  lpVersionInformation->EmulatedProcessorType = IA32_PROCESSOR; //We can only be running x86-32 code from Win16 or DOS

#if defined(COSVERSION_WIN16) //Running on some version of Windows                   

  DWORD dwVersion = ::GetVersion();
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
      if (IsWindows95SP1(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
        lpVersionInformation->wUnderlyingServicePackMajor = 1;
      else if (IsWindows95B(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
        lpVersionInformation->wUnderlyingServicePackMajor = 2;
      else if (IsWindows95C(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
      {
        lpVersionInformation->wUnderlyingServicePackMajor = 2; 
        lpVersionInformation->wUnderlyingServicePackMinor = 5;
      }  
      else if (IsWindows98SP1(osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber))
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

      //Get the Bing Edition details
      GetBingEditionIDFromRegistry(lpVersionInformation);
          
      //Determine if it is NT SP6a Vs SP6
      GetNTSP6aDetailsFromRegistry(lpVersionInformation, FALSE);

      //Determine if it is XP SP1a Vs SP1
      GetXPSP1aDetailsFromRegistry(lpVersionInformation, FALSE);

      //Determine the HAL details
      GetNTHALDetailsFromRegistry(lpVersionInformation);

      //Get the Product Suites installed
      GetProductSuiteDetailsFromRegistry(lpVersionInformation);
      GetTerminalServicesRemoteAdminModeDetailsFromRegistry(lpVersionInformation);
      GetMediaCenterDetails(lpVersionInformation);
      GetTabletPCDetails(lpVersionInformation);
      GetStarterEditionDetails(lpVersionInformation);
      GetR2Details(lpVersionInformation);
      GetProductInfo(lpVersionInformation);
    }
    else
      return FALSE;
       
    bFoundUnderlyingOS = TRUE;
  }
  else
  {
    //We failed to get GetVersionEx so try to GetVersion instead. We known that we must be on
    //Windows NT 3.5 or earlier since anything released later by MS had this function.
    DWORD dwVersion = ::GetVersion();
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
            lpVersionInformation->dwUnderlyingBuildNumber = 1212; //Windows 95 B [aka OSR2] reports DOS 7.03 from 16 bit code
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
#endif //#if defined(COSVERSION_WIN16) 

#endif //#if defined(COSVERSION_CE)

  return TRUE;
}

#if defined(COSVERSION_DOS)
BOOL COSVersion::GetInfoBySpawingWriteVer(COSVersion::LPOS_VERSION_INFO lpVersionInformation)
{
  //What will be the return value from this function (assume the worst)
  BOOL bSuccess = FALSE;

  //Form the command line we need to run writever with
  char* pszTempFilename = _tempnam(NULL, "DTW");
  if (pszTempFilename == NULL)
    return FALSE;
  char szCommandLine[256];
  szCommandLine[0] = '\0';
  sprintf(szCommandLine, "WriteVer.exe %s", pszTempFilename);

  //Try to spawn out writever utilty
  if (system(szCommandLine) != -1)
  {           
    //Open the file we need                 
    FILE* pOSFile = fopen(pszTempFilename, "r");
    if (pOSFile)
    {
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
                      lpVersionInformation->dwSuiteMask = atol(pszToken);
                      pszToken = strtok(NULL, pszSeparators);
                      if (pszToken)
                      {
                        lpVersionInformation->dwSuiteMask2 = atol(pszToken);
                        pszToken = strtok(NULL, pszSeparators);
                        if (pszToken)
                        {
                          lpVersionInformation->dwSuiteMask3 = atol(pszToken);
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
        }
      }
      
      //Don't forget to close the file
      fclose(pOSFile);
    }

    //and remove it now that we are finished with it
    remove(pszTempFilename);

    //Free up the memory we have used
    free(pszTempFilename);
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
#else
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
void COSVersion::GetNTSP6aDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bUpdateEmulatedAlso)
{
#if !defined(COSVERSION_CE)
  if ((lpVersionInformation->dwUnderlyingMajorVersion == 4) && (lpVersionInformation->wUnderlyingServicePackMajor == 6))
  {
    //Test for SP6 versus SP6a.
    HKEY hKey = NULL;
#if !defined(COSVERSION_WIN16)
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                  
    if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009"), &hKey) == ERROR_SUCCESS)
#endif //#if !defined(COSVERSION_WIN16)   
    {
      lpVersionInformation->wUnderlyingServicePackMinor = 1;
      if (bUpdateEmulatedAlso)
        lpVersionInformation->wEmulatedServicePackMinor = 1;

      //Don't forget to close the registry key we were using
      RegCloseKey(hKey);
    }
  }
#endif //#if !defined(COSVERSION_CE)
}

void COSVersion::GetXPSP1aDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bUpdateEmulatedAlso)
{
#if !defined(COSVERSION_CE)
  if ((lpVersionInformation->dwUnderlyingMajorVersion == 5) && (lpVersionInformation->dwUnderlyingMinorVersion != 0) && (lpVersionInformation->wUnderlyingServicePackMajor == 1))
  {
    //Test for SP1a versus SP1.
    HKEY hKey = NULL;
#if !defined(COSVERSION_WIN16)
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                  
    if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), &hKey) == ERROR_SUCCESS)
#endif //#if !defined(COSVERSION_WIN16)   
    {
      TCHAR sTemp[1024];
      sTemp[0] = _T('\0');
      DWORD dwBufLen = 1024 * sizeof(TCHAR);

#if !defined(COSVERSION_WIN16)
      if (::RegQueryValueEx(hKey, _T("SubVersionNumber"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#else
      if (RegQueryValueEx(hKey, _T("SubVersionNumber"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#endif //#if !defined(COSVERSION_WIN16)
      {
        if (_tcsicmp(sTemp, _T("a")) == 0)
        {
          lpVersionInformation->wUnderlyingServicePackMinor = 1;
          if (bUpdateEmulatedAlso)
            lpVersionInformation->wEmulatedServicePackMinor = 1;
        }
      }

      //Don't forget to close the registry key we were using
      RegCloseKey(hKey);
    }
  }
#endif //#if !defined(COSVERSION_CE)
}

COSVersion::OS_TYPE COSVersion::GetNTOSTypeFromRegistry()
{
  //What will be the return value from this method
  OS_TYPE osType = UnknownOSType;

  //Open and the product options key
  HKEY hKeyProductOptions = NULL;
#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_READ | KEY_WOW64_64KEY, &hKeyProductOptions) == ERROR_SUCCESS)
#else                                                                                                                               
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), &hKeyProductOptions) == ERROR_SUCCESS)
#endif //#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  {
    osType = GetNTOSTypeFromRegistry(hKeyProductOptions);

    //Don't forget to close the registry key we were using
    RegCloseKey(hKeyProductOptions);
  }

  return osType;
}

COSVersion::OS_TYPE COSVersion::GetNTOSTypeFromRegistry(HKEY hKeyProductOptions)
{
  //What will be the return value from this method
  OS_TYPE osType = UnknownOSType;

  TCHAR sTemp[1024];
  sTemp[0] = _T('\0');
  DWORD dwBufLen = 1024 * sizeof(TCHAR);

#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  if (::RegQueryValueEx(hKeyProductOptions, _T("ProductType"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#else
  if (RegQueryValueEx(hKeyProductOptions, _T("ProductType"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#endif //#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  {
    if (_tcsicmp(sTemp, _T("LANMANNT")) == 0)
      osType = DomainController;
    else if (_tcsicmp(sTemp, _T("SERVERNT")) == 0)
      osType = Server;
    else if (_tcsicmp(sTemp, _T("LANSECNT")) == 0)
      osType = DomainController;
    else if (_tcsicmp(sTemp, _T("WinNT")) == 0)
      osType = Workstation;
  }

  return osType;
}

void COSVersion::GetNTOSTypeFromRegistry(LPOS_VERSION_INFO lpVersionInformation, BOOL bOnlyUpdateDCDetails)
{
  //Open and the product options key
  HKEY hKey = NULL;
#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                                                                                                               
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), &hKey) == ERROR_SUCCESS)
#endif //#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  {
    TCHAR sTemp[1024];
    sTemp[0] = _T('\0');
    DWORD dwBufLen = 1024 * sizeof(TCHAR);

#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
    if (::RegQueryValueEx(hKey, _T("ProductType"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#else
    if (RegQueryValueEx(hKey, _T("ProductType"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#endif //#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
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

    //Don't forget to close the registry key we were using
    RegCloseKey(hKey);
  }
}

void COSVersion::GetBingEditionIDFromRegistry(LPOS_VERSION_INFO lpVersionInformation)
{
  //Open and the CurrentVersion key
  HKEY hKey = NULL;
#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                                                                                                               
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion"), &hKey) == ERROR_SUCCESS)
#endif //#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  {
    TCHAR sTemp[1024];
    sTemp[0] = _T('\0');
    DWORD dwBufLen = 1024 * sizeof(TCHAR);

#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
    if (::RegQueryValueEx(hKey, _T("EditionID"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#else
    if (RegQueryValueEx(hKey, _T("EditionID"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#endif //#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
    {
    #if defined(_tcsupr_s)
      _tcsupr_s(sTemp, sizeof(sTemp)/sizeof(TCHAR));
    #else
      _tcsupr(sTemp);
    #endif //#if defined(_tcsupr_s)
      if ((_tcsstr(sTemp, _T("CORECONNECTED")) != NULL) || (_tcsstr(sTemp, _T("CONNECTEDCORE")) != NULL))
        lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORECONNECTED;
    }

    //Don't forget to close the registry key we were using
    RegCloseKey(hKey);
  }
}

void COSVersion::GetProductSuiteDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation) 
{
  //Lookup the ProductOptions registry key
  HKEY hKey = NULL;
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                                                                                                         
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), &hKey) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  {
    //Determine required size of ProductSuite buffer.
    DWORD dwType = 0;
    DWORD dwSize = 0;
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    if (::RegQueryValueEx(hKey, _T("ProductSuite"), NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
#else
    if (RegQueryValueEx(hKey, _T("ProductSuite"), NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    {
      //Allocate buffer.
      LPTSTR lpszProductSuites = (LPTSTR) new BYTE[dwSize];

      //Retrieve array of product suite strings.
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
      if (::RegQueryValueEx(hKey, _T("ProductSuite"), NULL, &dwType, (LPBYTE) lpszProductSuites, &dwSize) == ERROR_SUCCESS)
#else                                                                                                     
      if (RegQueryValueEx(hKey, _T("ProductSuite"), NULL, &dwType, (LPBYTE) lpszProductSuites, &dwSize) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64) 
      {
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
      }

      //Don't forget to free up the resource we used
      delete [] lpszProductSuites;
    }

    //Don't forget to close the registry key we were using
    RegCloseKey(hKey);
  }
}

void COSVersion::GetTerminalServicesRemoteAdminModeDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation)
{
  //Lookup the TSAppCompat registry value
  HKEY hKey = NULL;
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Terminal Server"), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                                                                                                         
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Terminal Server"), &hKey) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  {
    DWORD dwTSAppCompat = 1;
    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    if (::RegQueryValueEx(hKey, _T("TSAppCompat"), NULL, &dwType, (LPBYTE) &dwTSAppCompat, &dwSize) == ERROR_SUCCESS)
#else                                                                                                     
    if (RegQueryValueEx(hKey, _T("TSAppCompat"), NULL, &dwType, (LPBYTE) &dwTSAppCompat, &dwSize) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64) 
    {
      if (dwTSAppCompat == 0)
        lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL;
    }

    //Don't forget to close the registry key we were using
    RegCloseKey(hKey);
  }
}

void COSVersion::GetMediaCenterDetails(LPOS_VERSION_INFO lpVersionInformation)
{
  //Try the system metrics call first and if that fails, as a fall back go looking directly in the registry
  if (!GetSystemMetrics(SM_MEDIACENTER))
  {
    //Lookup the WPA\MediaCenter\Installed registry value
    HKEY hKey = NULL;
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\WPA\\MediaCenter"), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                                                                                                         
    if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\WPA\\MediaCenter"), &hKey) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    {
      DWORD dwMCE = 0;
      DWORD dwType;
      DWORD dwSize = sizeof(DWORD);
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
      if (::RegQueryValueEx(hKey, _T("Installed"), NULL, &dwType, (LPBYTE) &dwMCE, &dwSize) == ERROR_SUCCESS)
#else                                                                                                     
      if (RegQueryValueEx(hKey, _T("Installed"), NULL, &dwType, (LPBYTE) &dwMCE, &dwSize) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64) 
      {
        if (dwMCE)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_MEDIACENTER;
      }

      //Don't forget to close the registry key we were using
      RegCloseKey(hKey);
    }
  }
  else
    lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_MEDIACENTER;
}

void COSVersion::GetProductInfo(LPOS_VERSION_INFO lpVersionInformation)
{
  DWORD dwProductType = 0;
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  HMODULE hKernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
  if (hKernel32 != NULL)
  {
#if defined(COSVERSION_CE)
    lpfnGetProductInfo pGetProductInfo = (lpfnGetProductInfo) GetProcAddress(hKernel32, L"GetProductInfo"); 
#else
    lpfnGetProductInfo pGetProductInfo = (lpfnGetProductInfo) GetProcAddress(hKernel32, "GetProductInfo"); 
#endif //#if defined(COSVERSION_CE)
    if (pGetProductInfo)
      pGetProductInfo(lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, lpVersionInformation->wUnderlyingServicePackMajor, 
      lpVersionInformation->wUnderlyingServicePackMinor, &dwProductType);
  }  
#else
  if (m_lpfnGetProductInfo)
    m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 5, 0x10, m_lpfnGetProductInfo, lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, 
    (DWORD) lpVersionInformation->wUnderlyingServicePackMajor, (DWORD) lpVersionInformation->wUnderlyingServicePackMinor, &dwProductType); 
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)     
      
  switch (dwProductType)
  {
    case PRODUCT_SB_SOLUTION_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STORAGE_SERVER_ESSENTIALS;
      break;
    }
    case PRODUCT_SB_SOLUTION_SERVER_EM:
    {
      //Note really sure what "PRODUCT_SB_SOLUTION_SERVER_EM", we just treat it as PRODUCT_SB_SOLUTION_SERVER
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STORAGE_SERVER_ESSENTIALS;
      break;
    }
    case PRODUCT_HOME_PREMIUM_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HOME_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_HOME_SERVER_PREMIUM;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PREMIUM;
      break;
    }
    case PRODUCT_STARTER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STARTER_EDITION;
      break;
    }
    case PRODUCT_HOME_BASIC_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HOME_BASIC;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      break;
    }
    case PRODUCT_HOME_BASIC:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HOME_BASIC;
      break;
    }
    case PRODUCT_HOME_PREMIUM_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HOME_PREMIUM;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PREMIUM;
      break;
    }
    case PRODUCT_HOME_PREMIUM:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HOME_PREMIUM;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PREMIUM;
      break;
    }
    case PRODUCT_BUSINESS_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_BUSINESS;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      break;
    }
    case PRODUCT_BUSINESS:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_BUSINESS;
      break;
    }
    case PRODUCT_ENTERPRISE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      break;
    }
    case PRODUCT_ENTERPRISE_EVALUATION:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      break;
    }
    case PRODUCT_ENTERPRISE_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      break;
    }
    case PRODUCT_ENTERPRISE_N_EVALUATION:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      break;
    }
    case PRODUCT_ULTIMATE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ULTIMATE;
      break;
    }
    case PRODUCT_ULTIMATE_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ULTIMATE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      break;
    }
    case PRODUCT_DATACENTER_SERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_DATACENTER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      break;
    }
    case PRODUCT_DATACENTER_SERVER_CORE_V: 
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_DATACENTER;
      break;
    }
    case PRODUCT_DATACENTER_SERVER_V: 
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_DATACENTER;
      break;
    }
    case PRODUCT_DATACENTER_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_DATACENTER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      break;
    }
    case PRODUCT_DATACENTER_EVALUATION_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_DATACENTER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      break;
    }
    case PRODUCT_ENTERPRISE_SERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      break;
    }
    case PRODUCT_ENTERPRISE_SERVER_CORE_V:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      break;
    }
    case PRODUCT_ENTERPRISE_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      break;
    }
    case PRODUCT_ENTERPRISE_SERVER_V:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      break;
    }
    case PRODUCT_ENTERPRISE_SERVER_IA64:
    {
  #if !defined(COSVERSION_CE)
      lpVersionInformation->UnderlyingProcessorType = IA64_PROCESSOR;
  #endif //#if !defined(COSVERSION_CE) 
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      break;
    }
    case PRODUCT_STANDARD_SERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      break;
    }
    case PRODUCT_STANDARD_SERVER_CORE_V:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      break;
    }
    case PRODUCT_STANDARD_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      break;
    }
    case PRODUCT_STANDARD_EVALUATION_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      break;
    }
    case PRODUCT_STANDARD_SERVER_V:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      break;
    }
    case PRODUCT_HYPERV:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_HYPERV_SERVER;
      break;
    }
    case PRODUCT_WEB_SERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_WEBEDITION;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      break;
    }
    case PRODUCT_WEB_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_WEBEDITION;
      break;
    }
    case PRODUCT_HOME_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HOME_SERVER;
      break;
    }
    case PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MANAGEMENT;
      break;
    }
    case PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORE;
      break;
    }
    case PRODUCT_SERVER_FOR_SB_SOLUTIONS: //deliberate fallthrough
    case PRODUCT_SERVER_FOR_SB_SOLUTIONS_EM: //deliberate fallthrough
    case PRODUCT_STANDARD_SERVER_SOLUTIONS:
    {
      //Could find no documentation on what these values actually represent
      break;
    }
    case PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC:
    {
      //Note really sure what "PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC", we just treat it as PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT 
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MANAGEMENT;
      break;
    }
    case PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL:
    {
      //Note really sure what "PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL", we just treat it as COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MESSAGING. It probably refers to the cancelled EBS R2 release
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MESSAGING;
      break;
    }
    case PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC:
    {
      //Note really sure what "PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC", we just treat it as COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MESSAGING. It probably refers to the cancelled EBS R2 release
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MESSAGING;
      break;
    }
    case PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MANAGEMENT;
      break;
    }
    case PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MESSAGING;
      break;
    }
    case PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_ESSENTIAL_BUSINESS_SERVER_SECURITY;
      break;
    }
    case PRODUCT_CLUSTER_SERVER_V:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CLUSTER_SERVER;
      break;
    }
    case PRODUCT_CLUSTER_SERVER:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CLUSTER_SERVER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      break;
    }
    case PRODUCT_SMALLBUSINESS_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SMALLBUSINESS;
      break;
    }
    case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SMALLBUSINESS;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_SMALLBUSINESS_PREMIUM;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PREMIUM;
      break;
    }
    case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SMALLBUSINESS;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_SMALLBUSINESS_PREMIUM;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PREMIUM;
      break;
    }
    case PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      break;
    }
    case PRODUCT_STORAGE_ENTERPRISE_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      break;
    }
    case PRODUCT_STORAGE_EXPRESS_SERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STORAGE_EXPRESS_SERVER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      break;
    }
    case PRODUCT_STORAGE_EXPRESS_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STORAGE_EXPRESS_SERVER;
      break;
    }
    case PRODUCT_STORAGE_STANDARD_SERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      break;
    }
    case PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      break;
    }
    case PRODUCT_STORAGE_STANDARD_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      break;
    }
    case PRODUCT_STORAGE_WORKGROUP_SERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STORAGE_WORKGROUP_SERVER;
      break;
    }
    case PRODUCT_STORAGE_WORKGROUP_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STORAGE_WORKGROUP_SERVER;
      break;
    }
    case PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STORAGE_WORKGROUP_SERVER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      break;
    }
    case PRODUCT_ENTERPRISE_E:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_E;
      break;
    }
    case PRODUCT_HOME_BASIC_E:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HOME_BASIC;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_E;
      break;
    }
    case PRODUCT_HOME_PREMIUM_E:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HOME_PREMIUM;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_E;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PREMIUM;
      break;
    }
    case PRODUCT_PROFESSIONAL:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      break;
    }
    case PRODUCT_PROFESSIONAL_E:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_E;
      break;
    }
    case PRODUCT_PROFESSIONAL_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      break;
    }
    case PRODUCT_SERVER_FOR_SMALLBUSINESS:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SMALLBUSINESS;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_HYPERV_TOOLS;
      break;
    }
    case PRODUCT_SERVER_FOR_SMALLBUSINESS_V:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SMALLBUSINESS;
      break;
    }
    case PRODUCT_SERVER_FOUNDATION:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_FOUNDATION;
      break;
    }
    case PRODUCT_SOLUTION_EMBEDDEDSERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_MULTIPOINT;
      break;
    }
    case PRODUCT_MULTIPOINT_STANDARD_SERVER:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_MULTIPOINT;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STANDARD;
      break;
    }
    case PRODUCT_MULTIPOINT_PREMIUM_SERVER:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_MULTIPOINT;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PREMIUM;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_MULTIPOINT_SERVER_PREMIUM;
      break;
    }
    case PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_SERVER_CORE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_MULTIPOINT;
      break;
    }
    case PRODUCT_STARTER_E:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STARTER_EDITION;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_E;
      break;
    }
    case PRODUCT_STARTER_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STARTER_EDITION;
      break;
    }
    case PRODUCT_ULTIMATE_E:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ULTIMATE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_E;
      break;
    }
    case PRODUCT_PRERELEASE:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PRERELEASE;
      break;
    }
    case PRODUCT_PRERELEASE_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PRERELEASE;
      break;
    }
    case PRODUCT_PRERELEASE_ARM:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PRERELEASE;
    #if !defined(COSVERSION_CE)
      lpVersionInformation->UnderlyingProcessorType = ARM_PROCESSOR;
    #endif //#if !defined(COSVERSION_CE) 
      break;
    }
    case PRODUCT_PROFESSIONAL_PLUS: //This was a SKU available during the Windows 8 Consumer Preview
    {
      //We just map to the "Windows 8 Pro" SKU
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      break;
    }
    case PRODUCT_THINPC:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_THINPC;
      break;
    }
    case PRODUCT_EMBEDDED_AUTOMOTIVE:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_AUTOMOTIVE;
      break;
    }
    case PRODUCT_EMBEDDED: //deliberate fallthrough
    case PRODUCT_EMBEDDED_A: //deliberate fallthrough
    case PRODUCT_EMBEDDED_E:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      break;
    }
    case PRODUCT_EMBEDDED_INDUSTRY: //deliberate fallthrough
    case PRODUCT_EMBEDDED_INDUSTRY_A: //deliberate fallthrough
    case PRODUCT_EMBEDDED_INDUSTRY_E: //deliberate fallthrough
    case PRODUCT_EMBEDDED_INDUSTRY_A_E:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_INDUSTRY;
      break;
    }
    case PRODUCT_CORE_ARM:
    {
    #if !defined(COSVERSION_CE)
      lpVersionInformation->UnderlyingProcessorType = ARM_PROCESSOR;
    #endif //#if !defined(COSVERSION_CE) 
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORE;
      break;
    }
    case PRODUCT_CORE:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORE;
      break;
    }
    case PRODUCT_CORE_COUNTRYSPECIFIC:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CHINA;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORE;
      break;
    }
    case PRODUCT_CORE_SINGLELANGUAGE:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_SINGLE_LANGUAGE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORE;
      break;
    }
    case PRODUCT_CORE_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORE;
      break;
    }
    case PRODUCT_PROFESSIONAL_WMC:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_MEDIACENTER;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      break;
    }
    case PRODUCT_PROFESSIONAL_EMBEDDED:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      break;
    }
    case PRODUCT_MOBILE_CORE:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_MOBILE;
      break;
    }
    case PRODUCT_EMBEDDED_INDUSTRY_EVAL:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_INDUSTRY;
      break;
    }
    case PRODUCT_EMBEDDED_INDUSTRY_E_EVAL:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_E;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_INDUSTRY;
      break;
    }
    case PRODUCT_EMBEDDED_EVAL:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      break;
    }
    case PRODUCT_EMBEDDED_E_EVAL:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_EMBEDDED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_E;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      break;
    }
    case PRODUCT_NANO_SERVER:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_NANO_SERVER;
      break;
    }
    case PRODUCT_CLOUD_STORAGE_SERVER:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STORAGE_SERVER;
      lpVersionInformation->dwSuiteMask3 |= COSVERSION_SUITE3_CLOUD_STORAGE_SERVER;
      break;
    }
    case PRODUCT_CORE_CONNECTED:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORECONNECTED;
      break;
    }
    case PRODUCT_PROFESSIONAL_STUDENT:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STUDENT;
      break;
    }
    case PRODUCT_CORE_CONNECTED_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORECONNECTED;
      break;
    }
    case PRODUCT_PROFESSIONAL_STUDENT_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_STUDENT;
      break;
    }
    case PRODUCT_CORE_CONNECTED_SINGLELANGUAGE:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORECONNECTED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_SINGLE_LANGUAGE;
      break;
    }
    case PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CORECONNECTED;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_CHINA;
      break;
    }
    case PRODUCT_CONNECTED_CAR:
    {
      lpVersionInformation->dwSuiteMask3 |= COSVERSION_SUITE3_CONNECTED_CAR;
      break;
    }
    case PRODUCT_INDUSTRY_HANDHELD:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_INDUSTRY;
      lpVersionInformation->dwSuiteMask3 |= COSVERSION_SUITE3_HANDHELD;
      break;
    }
    case PRODUCT_PPI_PRO:
    {
      lpVersionInformation->dwSuiteMask3 |= COSVERSION_SUITE3_PPI_PRO;
      break;
    }
    case PRODUCT_ARM64_SERVER:
    {
      lpVersionInformation->dwSuiteMask3 |= COSVERSION_SUITE3_ARM64_SERVER;
      break;
    }
    case PRODUCT_EDUCATION:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EDUCATION;
      break;
    }
    case PRODUCT_EDUCATION_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EDUCATION;
      break;
    }
    case PRODUCT_IOTUAP:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_IOT;
      break;
    }
    case PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER:
    {
      lpVersionInformation->dwSuiteMask3 |= COSVERSION_SUITE3_CLOUD_HOST_INFRASTRUCTURE_SERVER;
      break;
    }
    case PRODUCT_ENTERPRISE_S:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_S;
      break;
    }
    case PRODUCT_ENTERPRISE_S_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_S;
      break;
    }
    case PRODUCT_PROFESSIONAL_S:
    {
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_S;
      break;
    }
    case PRODUCT_PROFESSIONAL_S_N:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_S;
      break;
    }
    case PRODUCT_ENTERPRISE_S_EVALUATION:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_S;
      break;
    }
    case PRODUCT_ENTERPRISE_S_N_EVALUATION:
    {
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_N;
      lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_ENTERPRISE;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_EVALUATION;
      lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_S;
      break;
    }
    default:
    {
      break;
    }
  }

  //Synthesize the Professional value for XP and 2000
  if (IsWindows2000Professional(lpVersionInformation, TRUE) || IsWindowsXPProfessional(lpVersionInformation, TRUE))
    lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_PROFESSIONAL;
}

void COSVersion::GetTabletPCDetails(LPOS_VERSION_INFO lpVersionInformation)
{
  if (!GetSystemMetrics(SM_TABLETPC))
  {
    //Lookup the WPA\TablePC\Installed registry value
    HKEY hKey = NULL;
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\WPA\\TabletPC"), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                                                                                                         
    if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\WPA\\TabletPC"), &hKey) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
    {
      DWORD dwMCE = 0;
      DWORD dwType;
      DWORD dwSize = sizeof(DWORD);
#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
      if (::RegQueryValueEx(hKey, _T("Installed"), NULL, &dwType, (LPBYTE)&dwMCE, &dwSize) == ERROR_SUCCESS)
#else                                                                                                     
      if (RegQueryValueEx(hKey, _T("Installed"), NULL, &dwType, (LPBYTE)&dwMCE, &dwSize) == ERROR_SUCCESS)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64) 
      {
        if (dwMCE)
          lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_TABLETPC;
      }

      //Don't forget to close the registry key we were using
      RegCloseKey(hKey);
    }
  }
  else
    lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_TABLETPC;
}

void COSVersion::GetStarterEditionDetails(LPOS_VERSION_INFO lpVersionInformation)
{
  if (GetSystemMetrics(SM_STARTER))
    lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_STARTER_EDITION;
}

void COSVersion::GetR2Details(LPOS_VERSION_INFO lpVersionInformation)
{
  if (GetSystemMetrics(SM_SERVERR2))
    lpVersionInformation->dwSuiteMask |= COSVERSION_SUITE_R2_EDITION;
}

void COSVersion::GetNTHALDetailsFromRegistry(LPOS_VERSION_INFO lpVersionInformation)
{
#ifndef COSVERSION_CE
  HKEY hKey = NULL;
#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
#else                                  
  if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), &hKey) == ERROR_SUCCESS)
#endif //#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))   
  {
    TCHAR sTemp[1024];
    sTemp[0] = _T('\0');
    DWORD dwBufLen = 1024 * sizeof(TCHAR);

#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
    if (::RegQueryValueEx(hKey, _T("CurrentType"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#else
    if (RegQueryValueEx(hKey, _T("CurrentType"), NULL, NULL, (LPBYTE)sTemp, &dwBufLen) == ERROR_SUCCESS)
#endif //#if (defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64))
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

    //Don't forget to close the registry key we were using
    RegCloseKey(hKey);
  }
#endif //#ifndef COSVERSION_CE
}
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#endif //#if defined(COSVERSION_DOS)

#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64) 
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
BOOL COSVersion::GetNTCSDVersionFromRegistry(HKEY hKeyCurrentVersion, LPOS_VERSION_INFO lpVersionInformation, BOOL bEmulated)
{
  //What will be the return value from this method (Assume the worst)
  BOOL bSuccess = FALSE;

  BYTE byData[128];
  DWORD dwType;
  DWORD dwSize = sizeof(byData);
  bSuccess = ::RegQueryValueEx(hKeyCurrentVersion, _T("CSDVersion"), NULL, &dwType, byData, &dwSize) == ERROR_SUCCESS;
  if (bSuccess)
  {
    if (dwType == REG_SZ)
    {
      //Stored as a string on all other versions of NT
      if (bEmulated)
      {
        lpVersionInformation->wEmulatedServicePackMajor = GetNTServicePackFromCSDString((TCHAR*) byData);
        lpVersionInformation->wEmulatedServicePackMinor = 0;
      #if defined(_tcscpy_s)
        _tcscpy_s(lpVersionInformation->szEmulatedCSDVersion, sizeof(lpVersionInformation->szEmulatedCSDVersion)/sizeof(TCHAR), (TCHAR*) byData);
      #else
        _tcscpy(lpVersionInformation->szEmulatedCSDVersion, (TCHAR*) byData);
      #endif //#if defined(_tcscpy_s)
      }
      else
      {
        lpVersionInformation->wUnderlyingServicePackMajor = GetNTServicePackFromCSDString((TCHAR*) byData);
        lpVersionInformation->wUnderlyingServicePackMinor = 0;
      #if defined(_tcscpy_s) 
        _tcscpy_s(lpVersionInformation->szUnderlyingCSDVersion, sizeof(lpVersionInformation->szUnderlyingCSDVersion)/sizeof(TCHAR), (TCHAR*) byData);
      #else
        _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, (TCHAR*) byData);
      #endif //#if defined(_tcscpy_s) 
      }
    }
    else if (dwType == REG_DWORD)
    {
      //Handle the service pack number being stored as a DWORD which happens on NT 3.1
      if (bEmulated)
      {
        lpVersionInformation->wEmulatedServicePackMajor = HIBYTE((WORD) *((DWORD*) byData));
        lpVersionInformation->wEmulatedServicePackMinor = 0;
      #if defined(_tcscpy_s) 
        _tcscpy_s(lpVersionInformation->szEmulatedCSDVersion, sizeof(lpVersionInformation->szEmulatedCSDVersion)/sizeof(TCHAR), _T(""));
      #else
        _tcscpy(lpVersionInformation->szEmulatedCSDVersion, _T(""));
      #endif //#if defined(_tcscpy_s) 
      }
      else
      {
        lpVersionInformation->wUnderlyingServicePackMajor = HIBYTE((WORD) *((DWORD*) byData));
        lpVersionInformation->wUnderlyingServicePackMinor = 0;
      #if defined(_tcscpy_s) 
        _tcscpy_s(lpVersionInformation->szUnderlyingCSDVersion, sizeof(lpVersionInformation->szUnderlyingCSDVersion)/sizeof(TCHAR), _T(""));
      #else
        _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, _T(""));
      #endif //#if defined(_tcscpy_s)
      }
    }
  }

  return bSuccess;
}

DWORD COSVersion::GetNTCurrentBuildFromRegistry(HKEY hKeyCurrentVersion)
{
  //What will be the return value from this method
  DWORD dwCurrentBuild = 0;

  BYTE byData[128];
  DWORD dwType;
  DWORD dwSize = sizeof(byData);
  if (::RegQueryValueEx(hKeyCurrentVersion, _T("CurrentBuildNumber"), NULL, &dwType, byData, &dwSize) == ERROR_SUCCESS)
    dwCurrentBuild = _ttoi((TCHAR*) byData);

  return dwCurrentBuild;
}

BOOL COSVersion::GetNTCurrentBuildFromRegistry(DWORD& dwCurrentBuild)
{
  //What will be the return value from this method (assume the worst)
  BOOL bSuccess = FALSE;

  HKEY hCurrentVersion = NULL;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ | KEY_WOW64_64KEY, &hCurrentVersion) == ERROR_SUCCESS)
  {
    bSuccess = TRUE;

    //Delegate to the other version of this method
    dwCurrentBuild = GetNTCurrentBuildFromRegistry(hCurrentVersion);

    //Don't forget to close the registry key we were using
    RegCloseKey(hCurrentVersion);
  }

  return bSuccess;
}

BOOL COSVersion::GetNTCurrentVersionFromRegistry(HKEY hKeyCurrentVersion, DWORD& dwMajorVersion, DWORD& dwMinorVersion)
{
  //What will be the return value from this method (assume the worst)
  BOOL bSuccess = FALSE;

  TCHAR szData[128];
  szData[0] = _T('\0');
  DWORD dwType;
  DWORD dwSize = sizeof(szData) / sizeof(TCHAR);
  bSuccess = ::RegQueryValueEx(hKeyCurrentVersion, _T("CurrentVersion"), NULL, &dwType, (BYTE*) szData, &dwSize) == ERROR_SUCCESS;
  if (bSuccess)
  {
    _stscanf(szData, _T("%u.%u"), &dwMajorVersion, &dwMinorVersion);
  }

  return bSuccess;
}

BOOL COSVersion::GetNTRTLVersion(LPOS_VERSION_INFO lpVersionInformation)
{
  //Get the NTDLL handle
  HMODULE hNTDLL = GetModuleHandle(_T("NTDLL.DLL"));
  if (hNTDLL == NULL)
    return FALSE;

  //Get the function pointer to the native mode API "RtlGetVersion"
#if defined(COSVERSION_CE)
  lpfnRtlGetVersion lpRtlGetVersion = (lpfnRtlGetVersion) GetProcAddress(hNTDLL, L"RtlGetVersion");
#else
  lpfnRtlGetVersion lpRtlGetVersion = (lpfnRtlGetVersion) GetProcAddress(hNTDLL, "RtlGetVersion");
#endif //#if defined(COSVERSION_CE)
  if (lpRtlGetVersion == NULL)
    return FALSE;

  //Call through the function pointer
  RTL_OSVERSIONINFOEXW rtlosviex;
  rtlosviex.dwOSVersionInfoSize = sizeof(rtlosviex);
  LONG nStatus = lpRtlGetVersion(&rtlosviex);
  if (nStatus == STATUS_SUCCESS)
  {
    //Copy the values from the RTL struct to the output parameter struct
    lpVersionInformation->dwUnderlyingBuildNumber = rtlosviex.dwBuildNumber;
    lpVersionInformation->dwUnderlyingMajorVersion = rtlosviex.dwMajorVersion;
    lpVersionInformation->dwUnderlyingMinorVersion = rtlosviex.dwMinorVersion;

    //Also transfer the service pack string value to the output parameter struct
  #if defined(_UNICODE)
  #if defined(wcscpy_s)
    wcscpy_s(lpVersionInformation->szUnderlyingCSDVersion, sizeof(lpVersionInformation->szUnderlyingCSDVersion)/sizeof(wchar_t), rtlosviex.szCSDVersion);
  #else
    wcscpy(lpVersionInformation->szUnderlyingCSDVersion, rtlosviex.szCSDVersion);
  #endif //#if defined(wcscpy_s)
  #else
    WideCharToMultiByte(CP_ACP, 0, rtlosviex.szCSDVersion, -1, lpVersionInformation->szUnderlyingCSDVersion, 
                        sizeof(lpVersionInformation->szUnderlyingCSDVersion), NULL, NULL);
  #endif //#if defined(_UNICODE)

    //Convert the CSD string into service pack values
    lpVersionInformation->wUnderlyingServicePackMajor = GetNTServicePackFromCSDString(lpVersionInformation->szUnderlyingCSDVersion);
    lpVersionInformation->wUnderlyingServicePackMinor = 0;
  }

  return (nStatus == STATUS_SUCCESS);
}

BOOL COSVersion::GetWindows8Point1Or2012R2Update(LPOS_VERSION_INFO lpVersionInformation)
{
  //What will be the return value from this method (assume the worst)
  BOOL bSuccess = FALSE;
  
  //Form the location of HAL.dll
  TCHAR szFileName[_MAX_PATH];
  szFileName[0] = _T('\0');
  if (GetSystemDirectory(szFileName, _MAX_PATH) == 0)
    return FALSE;
  #pragma warning(suppress: 6102)
  _tcscat_s(szFileName, sizeof(szFileName) / sizeof(TCHAR), _T("\\HAL.DLL"));

  //Check the size of the version info
  DWORD dwHandle = 0;
  DWORD dwSize = GetFileVersionInfoSize(szFileName, &dwHandle);
  if (dwSize)
  {
    //Alocate some memory to contain the version info
    BYTE* pbyData = static_cast<BYTE*>(HeapAlloc(GetProcessHeap(), 0, dwSize));
    if (pbyData == NULL)
      return FALSE;

    //Get the version info
    if (GetFileVersionInfo(szFileName, 0, dwSize, pbyData))
    {
      //Get the fixed size version info data
      UINT nLen = 0;
      VS_FIXEDFILEINFO* pffi = NULL;
      if (VerQueryValue(pbyData, _T("\\"), reinterpret_cast<LPVOID*>(&pffi), &nLen))
      {
        if (pffi->dwFileVersionLS >= MAKELONG(17031, 9600)) //Windows 8.1 / 2012 R2 Update has a HAL.DLL with a file version of 6.3.9600.17031
          lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_WINDOWS812012R2UPDATE;

        bSuccess = TRUE;
      }
    }

    //Free up the heap memory used
    HeapFree(GetProcessHeap(), 0, pbyData);
  }

  return bSuccess;
}

void COSVersion::_GetVersion(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber, TCHAR* szCSDVersion, WORD wServicePackMajor, 
                             WORD wServicePackMinor, DWORD dwPlatformId, BOOL bOnlyUpdateDCDetails, LPOS_VERSION_INFO lpVersionInformation)
{
  lpVersionInformation->dwEmulatedMajorVersion = dwMajorVersion;
  lpVersionInformation->dwEmulatedMinorVersion = dwMinorVersion;
  lpVersionInformation->dwEmulatedBuildNumber = LOWORD(dwBuildNumber);
#if defined(_tcscpy_s)
  _tcscpy_s(lpVersionInformation->szEmulatedCSDVersion, sizeof(lpVersionInformation->szEmulatedCSDVersion)/sizeof(TCHAR), szCSDVersion);
#else
  _tcscpy(lpVersionInformation->szEmulatedCSDVersion, szCSDVersion);
#endif //#if defined(_tcscpy_s)
  lpVersionInformation->wEmulatedServicePackMajor = wServicePackMajor;
  lpVersionInformation->wEmulatedServicePackMinor = wServicePackMinor;
        
  //Determine the Domain Controller Type
  GetNTOSTypeFromRegistry(lpVersionInformation, bOnlyUpdateDCDetails);

  //Get the Bing Edition details
  GetBingEditionIDFromRegistry(lpVersionInformation);

  //Get the media center details
  GetMediaCenterDetails(lpVersionInformation);

  //Get the tablet PC details
  GetTabletPCDetails(lpVersionInformation);

  //Get the Starter Edition details
  GetStarterEditionDetails(lpVersionInformation);

  //Get the R2 details
  GetR2Details(lpVersionInformation);

  //Explicitly map the win32 dwPlatformId to our own values 
  lpVersionInformation->EmulatedPlatform = MapWin32PlatformId(dwPlatformId);

  //Update the underlying values
  lpVersionInformation->dwUnderlyingMajorVersion = lpVersionInformation->dwEmulatedMajorVersion; 
  lpVersionInformation->dwUnderlyingMinorVersion = lpVersionInformation->dwEmulatedMinorVersion; 
  lpVersionInformation->dwUnderlyingBuildNumber = lpVersionInformation->dwEmulatedBuildNumber;
  lpVersionInformation->UnderlyingPlatform = lpVersionInformation->EmulatedPlatform;
#if defined(_tcscpy_s)
  _tcscpy_s(lpVersionInformation->szUnderlyingCSDVersion, sizeof(lpVersionInformation->szUnderlyingCSDVersion)/sizeof(TCHAR), lpVersionInformation->szEmulatedCSDVersion);
#else
  _tcscpy(lpVersionInformation->szUnderlyingCSDVersion, lpVersionInformation->szEmulatedCSDVersion);
#endif //#if defined(_tcscpy_s)
  lpVersionInformation->wUnderlyingServicePackMajor = lpVersionInformation->wEmulatedServicePackMajor;
  lpVersionInformation->wUnderlyingServicePackMinor = lpVersionInformation->wEmulatedServicePackMinor;

  //Get the product info details
  GetProductInfo(lpVersionInformation);

  //Determine if it is NT SP6a Vs SP6
  GetNTSP6aDetailsFromRegistry(lpVersionInformation, TRUE);

  //Determine if it is XP SP1a Vs SP1
  GetXPSP1aDetailsFromRegistry(lpVersionInformation, TRUE);

  //Determine the HAL details
  GetNTHALDetailsFromRegistry(lpVersionInformation);

  //Handle the special case of the Platform Id being Win32s
  if (dwPlatformId == VER_PLATFORM_WIN32s)
  {
    lpVersionInformation->EmulatedPlatform = WindowsNT;
    lpVersionInformation->dwSuiteMask2 |= COSVERSION_SUITE2_WIN32S;

    lpVersionInformation->dwUnderlyingMajorVersion = 3; 
    lpVersionInformation->dwUnderlyingMinorVersion = 10; 
    lpVersionInformation->dwUnderlyingBuildNumber = 0;
    lpVersionInformation->UnderlyingPlatform = Windows3x;
  }

  //Get the processor details                                       
  GetProcessorType(lpVersionInformation);

  //Call "RTLGetVersion" to get true underlying version details
  GetNTRTLVersion(lpVersionInformation);

  //Also check if Windows 8.1 / Windows 2012 R2 Update is installed (Details at http://technet.microsoft.com/en-us/library/dn645472.aspx)
  BOOL bIsWindows8Point1OrWindowsServer2012R2 = IsWindows8Point1OrWindowsServer2012R2(lpVersionInformation, TRUE);
  if (bIsWindows8Point1OrWindowsServer2012R2)
    GetWindows8Point1Or2012R2Update(lpVersionInformation);
}
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#endif //#if defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)

#if defined(COSVERSION_WIN16)
BOOL COSVersion::IsWindowsForWorkgroups(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
#else
BOOL COSVersion::IsWindowsForWorkgroups(LPOS_VERSION_INFO /*lpVersionInformation*/, BOOL /*bCheckUnderlying*/)
#endif //#if defined(CSOVERSION_WIN16)
{
#if defined(COSVERSION_WIN16)
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == Windows3x && WFWLoaded());
  else
    return (lpVersionInformation->EmulatedPlatform == Windows3x && WFWLoaded());
#else
  return FALSE; //Note that I could not find any way to differentiate between flavours of Windows 3.1 from Win32s
#endif //#if defined(COSVERSION_WIN16)
}

BOOL COSVersion::IsWindowsCE(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == WindowsCE);
  else
    return (lpVersionInformation->EmulatedPlatform == WindowsCE);
}

BOOL COSVersion::IsWindowsCENET(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  //Windows CE. NET is any version of CE 4.X where X > 0		
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsCE) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 4) &&
    (lpVersionInformation->dwUnderlyingMinorVersion > 0));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsCE) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 4) &&
    (lpVersionInformation->dwEmulatedMinorVersion > 0));
}

BOOL COSVersion::IsWindowsEmbeddedCompact(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  //Windows Embedded Compact is any version of CE 7.X
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsCE) &&
    (lpVersionInformation->dwUnderlyingMajorVersion >= 7));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsCE) &&
    (lpVersionInformation->dwEmulatedMajorVersion >= 7));
}

BOOL COSVersion::IsWindows95(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == Windows9x) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 4) &&
    (lpVersionInformation->dwUnderlyingMinorVersion == 0) &&
    (lpVersionInformation->dwUnderlyingBuildNumber == 950));
  else
    return ((lpVersionInformation->EmulatedPlatform == Windows9x) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 4) &&
    (lpVersionInformation->dwEmulatedMinorVersion == 0) &&
    (lpVersionInformation->dwEmulatedBuildNumber == 950));
}

BOOL COSVersion::IsWindows95SP1(DWORD dwMajorVersion, DWORD /*dwMinorVersion*/, DWORD dwBuildNumber)
{
  return (dwMajorVersion == 4) && (dwBuildNumber > 950) && (dwBuildNumber <= 1080);
}

BOOL COSVersion::IsWindows95SP1(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == Windows9x) &&
    IsWindows95SP1(lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, lpVersionInformation->dwUnderlyingBuildNumber));
  else
    return ((lpVersionInformation->EmulatedPlatform == Windows9x) &&
    IsWindows95SP1(lpVersionInformation->dwEmulatedMajorVersion, lpVersionInformation->dwEmulatedMinorVersion, lpVersionInformation->dwEmulatedBuildNumber));
}

BOOL COSVersion::IsWindows95B(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber)
{
  return (dwMajorVersion == 4) && (dwMinorVersion < 10) && (dwBuildNumber > 1080) && (dwBuildNumber < 1214);
}

BOOL COSVersion::IsWindows95B(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == Windows9x) &&
    IsWindows95B(lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, lpVersionInformation->dwUnderlyingBuildNumber));
  else
    return ((lpVersionInformation->EmulatedPlatform == Windows9x) &&
    IsWindows95B(lpVersionInformation->dwEmulatedMajorVersion, lpVersionInformation->dwEmulatedMinorVersion, lpVersionInformation->dwEmulatedBuildNumber));
}

BOOL COSVersion::IsWindows95C(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber)
{
  return (dwMajorVersion == 4) && (dwMinorVersion < 10) && (dwBuildNumber >= 1214);
}

BOOL COSVersion::IsWindows95C(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == Windows9x) &&
    IsWindows95C(lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, lpVersionInformation->dwUnderlyingBuildNumber));
  else
    return ((lpVersionInformation->EmulatedPlatform == Windows9x) &&
    IsWindows95C(lpVersionInformation->dwEmulatedMajorVersion, lpVersionInformation->dwEmulatedMinorVersion, lpVersionInformation->dwEmulatedBuildNumber));
}

BOOL COSVersion::IsWindows98(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber)
{
  return (dwMajorVersion == 4) && (dwMinorVersion == 10) && (dwBuildNumber == 1998);
}

BOOL COSVersion::IsWindows98(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == Windows9x) &&
    IsWindows98(lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, lpVersionInformation->dwUnderlyingBuildNumber));
  else
    return ((lpVersionInformation->EmulatedPlatform == Windows9x) &&
    IsWindows98(lpVersionInformation->dwEmulatedMajorVersion, lpVersionInformation->dwEmulatedMinorVersion, lpVersionInformation->dwEmulatedBuildNumber));
}

BOOL COSVersion::IsWindows98SP1(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber)
{
  return (dwMajorVersion == 4) && (dwMinorVersion == 10) && (dwBuildNumber > 1998) && (dwBuildNumber < 2183);
}

BOOL COSVersion::IsWindows98SP1(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == Windows9x) &&
    IsWindows98SP1(lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, lpVersionInformation->dwUnderlyingBuildNumber));
  else
    return ((lpVersionInformation->EmulatedPlatform == Windows9x) &&
    IsWindows98SP1(lpVersionInformation->dwEmulatedMajorVersion, lpVersionInformation->dwEmulatedMinorVersion, lpVersionInformation->dwEmulatedBuildNumber));
}

BOOL COSVersion::IsWindows98SE(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber)
{
  return (dwMajorVersion == 4) && (dwMinorVersion == 10) && (dwBuildNumber >= 2183);
}

BOOL COSVersion::IsWindows98SE(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == Windows9x) &&
    IsWindows98SE(lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, lpVersionInformation->dwUnderlyingBuildNumber));
  else
    return ((lpVersionInformation->EmulatedPlatform == Windows9x) &&
    IsWindows98SE(lpVersionInformation->dwEmulatedMajorVersion, lpVersionInformation->dwEmulatedMinorVersion, lpVersionInformation->dwEmulatedBuildNumber));
}

BOOL COSVersion::IsWindowsME(DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD /*dwBuildNumber*/)
{
  return (dwMajorVersion == 4) && (dwMinorVersion == 90);
}

BOOL COSVersion::IsWindowsME(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == Windows9x) &&
    IsWindowsME(lpVersionInformation->dwUnderlyingMajorVersion, lpVersionInformation->dwUnderlyingMinorVersion, lpVersionInformation->dwUnderlyingBuildNumber));
  else
    return ((lpVersionInformation->EmulatedPlatform == Windows9x) &&
    IsWindowsME(lpVersionInformation->dwEmulatedMajorVersion, lpVersionInformation->dwEmulatedMinorVersion, lpVersionInformation->dwEmulatedBuildNumber));
}

BOOL COSVersion::IsWindowsNT31(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsNT) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 3) &&
    (lpVersionInformation->dwUnderlyingMinorVersion == 10));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsNT) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 3) &&
    (lpVersionInformation->dwEmulatedMinorVersion == 10));
}

BOOL COSVersion::IsWindowsNT35(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsNT) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 3) &&
    (lpVersionInformation->dwUnderlyingMinorVersion == 50));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsNT) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 3) &&
    (lpVersionInformation->dwEmulatedMinorVersion == 50));
}

BOOL COSVersion::IsWindowsNT351(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsNT) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 3) &&
    (lpVersionInformation->dwUnderlyingMinorVersion == 51));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsNT) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 3) &&
    (lpVersionInformation->dwEmulatedMinorVersion == 51));
}

BOOL COSVersion::IsWindowsNT4(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsNT) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 4));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsNT) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 4));
}

BOOL COSVersion::IsWindows2000(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsNT) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 5) &&
    (lpVersionInformation->dwUnderlyingMinorVersion == 0));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsNT) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 5) &&
    (lpVersionInformation->dwEmulatedMinorVersion == 0));
}

BOOL COSVersion::IsWindowsXPOrWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsNT) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 5) &&
    (lpVersionInformation->dwUnderlyingMinorVersion != 0));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsNT) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 5) &&
    (lpVersionInformation->dwEmulatedMinorVersion != 0));
}

BOOL COSVersion::IsWindowsVistaOrWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsNT) &&
    (lpVersionInformation->dwUnderlyingMajorVersion == 6) &&
    (lpVersionInformation->dwUnderlyingMinorVersion == 0));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsNT) &&
    (lpVersionInformation->dwEmulatedMajorVersion == 6) &&
    (lpVersionInformation->dwEmulatedMinorVersion == 0));
}

BOOL COSVersion::IsWindowsVista(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsVistaOrWindowsServer2008(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsWindows7OrWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return ((lpVersionInformation->UnderlyingPlatform == WindowsNT) &&
            (lpVersionInformation->dwUnderlyingMajorVersion == 6) && (lpVersionInformation->dwUnderlyingMinorVersion == 1));
  else
    return ((lpVersionInformation->EmulatedPlatform == WindowsNT) &&
            (lpVersionInformation->dwEmulatedMajorVersion == 6) && (lpVersionInformation->dwEmulatedMinorVersion == 1));
}

BOOL COSVersion::IsWindows8OrWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
            (lpVersionInformation->dwUnderlyingMajorVersion == 6) && (lpVersionInformation->dwUnderlyingMinorVersion == 2));
  else
    return (lpVersionInformation->EmulatedPlatform == WindowsNT &&
            (lpVersionInformation->dwEmulatedMajorVersion == 6) && (lpVersionInformation->dwEmulatedMinorVersion == 2));
}

BOOL COSVersion::IsWindows8Point1OrWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
            ((lpVersionInformation->dwUnderlyingMajorVersion == 6) && lpVersionInformation->dwUnderlyingMinorVersion == 3));
  else
    return (lpVersionInformation->EmulatedPlatform == WindowsNT &&
            ((lpVersionInformation->dwEmulatedMajorVersion == 6) && lpVersionInformation->dwEmulatedMinorVersion == 3));
}

BOOL COSVersion::IsWindows10OrWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
            ((lpVersionInformation->dwUnderlyingMajorVersion > 6) ||
            ((lpVersionInformation->dwUnderlyingMajorVersion == 6) && lpVersionInformation->dwUnderlyingMinorVersion >= 4)));
  else
    return (lpVersionInformation->EmulatedPlatform == WindowsNT &&
            ((lpVersionInformation->dwEmulatedMajorVersion > 6) ||
            ((lpVersionInformation->dwEmulatedMajorVersion == 6) && lpVersionInformation->dwEmulatedMinorVersion >= 4)));
}

BOOL COSVersion::IsWindows10(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows10OrWindowsServer2016(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsWindows8Point1(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8Point1OrWindowsServer2012R2(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsWindows8Point1Update(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8Point1(lpVersionInformation, bCheckUnderlying) && IsWindows8Point1Or2012R2Update(lpVersionInformation);
}

BOOL COSVersion::IsWindows8(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8OrWindowsServer2012(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsWindows7(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows7OrWindowsServer2008R2(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsWindowsXP(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPPersonal(lpVersionInformation, bCheckUnderlying) || IsWindowsXPProfessional(lpVersionInformation, bCheckUnderlying);
}

BOOL COSVersion::IsWindowsRT(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
#if !defined(COSVERSION_CE)
  return IsWindows8(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->UnderlyingProcessorType == ARM_PROCESSOR);
#else
  return FALSE;
#endif //#if !defined(COSVERSION_CE)
}

WORD COSVersion::GetNTServicePackFromCSDString(LPCTSTR pszCSDVersion)
{
  WORD wServicePack = 0;
  if (_tcslen(pszCSDVersion))
  {
    //Parse out the CSDVersion string
    int i = 0;
    while (pszCSDVersion[i] != _T('\0') && !_istdigit((int)pszCSDVersion[i]))
      i++;
    wServicePack = (WORD)(_ttoi(&pszCSDVersion[i]));
  }

  return wServicePack;
}

BOOL COSVersion::IsWindows30(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == Windows3x &&
    lpVersionInformation->dwUnderlyingMinorVersion == 0);
  else
    return (lpVersionInformation->EmulatedPlatform == Windows3x &&
    lpVersionInformation->dwEmulatedMinorVersion == 0);
}

BOOL COSVersion::IsWindows31(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == Windows3x &&
    lpVersionInformation->dwUnderlyingMinorVersion == 10);
  else
    return (lpVersionInformation->EmulatedPlatform == Windows3x &&
    lpVersionInformation->dwEmulatedMinorVersion == 10);
}

BOOL COSVersion::IsWindows311(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == Windows3x &&
    lpVersionInformation->dwUnderlyingMinorVersion == 11);
  else
    return (lpVersionInformation->EmulatedPlatform == Windows3x &&
    lpVersionInformation->dwEmulatedMinorVersion == 11);
}

BOOL COSVersion::IsNTPreWin2k(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
    return (lpVersionInformation->UnderlyingPlatform == WindowsNT &&
            lpVersionInformation->dwUnderlyingMajorVersion <= 4);
  else
    return (lpVersionInformation->EmulatedPlatform == WindowsNT &&
            lpVersionInformation->dwEmulatedMajorVersion <= 4);
}

BOOL COSVersion::IsNTPDC(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return (IsNTPreWin2k(lpVersionInformation, bCheckUnderlying)) && 
         (lpVersionInformation->OSType == DomainController) &&
         ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_PRIMARY_DOMAIN_CONTROLLER) != 0);
}

BOOL COSVersion::IsNTStandAloneServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsNTPreWin2k(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server);
}

BOOL COSVersion::IsNTBDC(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsNTPreWin2k(lpVersionInformation, bCheckUnderlying) && 
         (lpVersionInformation->OSType == DomainController) && 
         ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_BACKUP_DOMAIN_CONTROLLER) != 0);
}

BOOL COSVersion::IsNTWorkstation(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsNTPreWin2k(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsTerminalServices(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_TERMINAL) != 0);
}

BOOL COSVersion::IsEmbedded(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_EMBEDDED) != 0);
}

BOOL COSVersion::IsTerminalServicesInRemoteAdminMode(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_REMOTEADMINMODE_TERMINAL) != 0);
}

BOOL COSVersion::IsSmallBusinessServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_SMALLBUSINESS) != 0);
}

BOOL COSVersion::IsSmallBusinessServerPremium(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_SMALLBUSINESS_PREMIUM) != 0);
}

BOOL COSVersion::IsNTEnterpriseServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return (IsNTPreWin2k(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsNTDatacenterServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsNTPreWin2k(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0);
}

BOOL COSVersion::IsWindows2000Professional(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows2000(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Workstation);
}

BOOL COSVersion::IsWindows2000Server(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows2000(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server);
}

BOOL COSVersion::IsWindows2000AdvancedServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows2000Server(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsWindows2000DatacenterServer(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return (IsWindows2000(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0);
}

BOOL COSVersion::IsWindows2000DomainController(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows2000(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == DomainController);
}

BOOL COSVersion::IsWindowsXPPersonal(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_PERSONAL) != 0);
}

BOOL COSVersion::IsWindowsXPProfessional(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Workstation) && !IsWindowsXPPersonal(lpVersionInformation, bCheckUnderlying);
}

BOOL COSVersion::IsWebWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_WEBEDITION) != 0) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) == 0);
}

BOOL COSVersion::IsWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) == 0);
}

BOOL COSVersion::IsEnterpriseWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsServer2003(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsDatacenterWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) == 0);
}

BOOL COSVersion::IsDomainControllerWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == DomainController) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) == 0);
}

BOOL COSVersion::IsStandardWindowsServer2003(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STANDARD) != 0) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) == 0);
}

BOOL COSVersion::IsWebWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_WEBEDITION) != 0) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) != 0);
}

BOOL COSVersion::IsWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) != 0);
}

BOOL COSVersion::IsEnterpriseWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsServer2003R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsStandardWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STANDARD) != 0) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) != 0);
}

BOOL COSVersion::IsDatacenterWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) != 0);
}

BOOL COSVersion::IsDomainControllerWindowsServer2003R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsXPOrWindowsServer2003(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == DomainController) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_R2_EDITION) != 0);
}

BOOL COSVersion::IsWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsVistaOrWindowsServer2008(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsWebWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsVistaOrWindowsServer2008(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_WEBEDITION) != 0); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsEnterpriseWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsServer2008(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsStandardWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsVistaOrWindowsServer2008(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STANDARD) != 0); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsDatacenterWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsVistaOrWindowsServer2008(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsDomainControllerWindowsServer2008(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsVistaOrWindowsServer2008(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == DomainController); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsWebWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows7OrWindowsServer2008R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_WEBEDITION) != 0); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows7OrWindowsServer2008R2(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsEnterpriseWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsServer2008R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsStandardWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows7OrWindowsServer2008R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STANDARD) != 0); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsDatacenterWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows7OrWindowsServer2008R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsDomainControllerWindowsServer2008R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows7OrWindowsServer2008R2(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == DomainController); //Note there is no need to check the COSVERSION_SUITE_R2EDITION flag here
}

BOOL COSVersion::IsWebWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8OrWindowsServer2012(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_WEBEDITION) != 0);
}

BOOL COSVersion::IsWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8OrWindowsServer2012(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server);
}

BOOL COSVersion::IsEnterpriseWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsServer2012(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsStandardWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8OrWindowsServer2012(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STANDARD) != 0);
}

BOOL COSVersion::IsDatacenterWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8OrWindowsServer2012(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0);
}

BOOL COSVersion::IsDomainControllerWindowsServer2012(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8OrWindowsServer2012(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == DomainController);
}

BOOL COSVersion::IsWebWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8Point1OrWindowsServer2012R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_WEBEDITION) != 0);
}

BOOL COSVersion::IsWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8Point1OrWindowsServer2012R2(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server);
}

BOOL COSVersion::IsWindowsServer2012R2Update(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsServer2012R2(lpVersionInformation, bCheckUnderlying) && IsWindows8Point1Or2012R2Update(lpVersionInformation);
}

BOOL COSVersion::IsEnterpriseWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsServer2012R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsStandardWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8Point1OrWindowsServer2012R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STANDARD) != 0);
}

BOOL COSVersion::IsDatacenterWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8Point1OrWindowsServer2012R2(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0);
}

BOOL COSVersion::IsDomainControllerWindowsServer2012R2(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows8Point1OrWindowsServer2012R2(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == DomainController);
}

BOOL COSVersion::IsWebWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows10OrWindowsServer2016(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_WEBEDITION) != 0);
}

BOOL COSVersion::IsWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows10OrWindowsServer2016(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == Server);
}

BOOL COSVersion::IsEnterpriseWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindowsServer2016(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsStandardWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows10OrWindowsServer2016(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STANDARD) != 0);
}

BOOL COSVersion::IsDatacenterWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows10OrWindowsServer2016(lpVersionInformation, bCheckUnderlying) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_DATACENTER) != 0);
}

BOOL COSVersion::IsDomainControllerWindowsServer2016(LPOS_VERSION_INFO lpVersionInformation, BOOL bCheckUnderlying)
{
  return IsWindows10OrWindowsServer2016(lpVersionInformation, bCheckUnderlying) && (lpVersionInformation->OSType == DomainController);
}

BOOL COSVersion::IsMediaCenter(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_MEDIACENTER) != 0);
}

BOOL COSVersion::IsTabletPC(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_TABLETPC) != 0);
}

BOOL COSVersion::IsStarterEdition(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_STARTER_EDITION) != 0);
}

BOOL COSVersion::IsComputeClusterServerEdition(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_COMPUTE_SERVER) != 0);
}

BOOL COSVersion::IsHomeServerEdition(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_HOME_SERVER) != 0);
}

BOOL COSVersion::IsHomeServerPremiumEdition(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_HOME_SERVER_PREMIUM) != 0);
}

BOOL COSVersion::IsSecurityAppliance(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_SECURITY_APPLIANCE) != 0);
}

BOOL COSVersion::IsBackOffice(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_BACKOFFICE) != 0);
}

BOOL COSVersion::IsHomeBasic(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_HOME_BASIC) != 0);
}

BOOL COSVersion::IsHomeBasicPremium(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_HOME_PREMIUM) != 0);
}

BOOL COSVersion::IsBusiness(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_BUSINESS) != 0);
}

BOOL COSVersion::IsProfessional(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_PROFESSIONAL) != 0);
}

BOOL COSVersion::IsEnterprise(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsUltimate(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ULTIMATE) != 0);
}

BOOL COSVersion::IsPersonal(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_PERSONAL) != 0);
}

BOOL COSVersion::IsNEdition(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_N) != 0);
}

BOOL COSVersion::IsEEdition(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_E) != 0);
}

BOOL COSVersion::IsHyperVTools(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_HYPERV_TOOLS) != 0);
}

BOOL COSVersion::IsHyperVServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_HYPERV_SERVER) != 0);
}

BOOL COSVersion::IsServerCore(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_SERVER_CORE) != 0);
}

BOOL COSVersion::IsMultiPointServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_MULTIPOINT) != 0);
}

BOOL COSVersion::IsServerFoundation(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_FOUNDATION) != 0);
}

BOOL COSVersion::IsUniprocessorFree(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_UNIPROCESSOR_FREE) != 0);
}

BOOL COSVersion::IsUniprocessorChecked(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_UNIPROCESSOR_CHECKED) != 0);
}

BOOL COSVersion::IsMultiprocessorFree(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_MULTIPROCESSOR_FREE) != 0);
}

BOOL COSVersion::IsMultiprocessorChecked(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_MULTIPROCESSOR_CHECKED) != 0);
}

BOOL COSVersion::IsEssentialBusinessServerManagement(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MANAGEMENT) != 0);
}

BOOL COSVersion::IsEssentialBusinessServerMessaging(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ESSENTIAL_BUSINESS_SERVER_MESSAGING) != 0);
}

BOOL COSVersion::IsEssentialBusinessServerSecurity(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_ESSENTIAL_BUSINESS_SERVER_SECURITY) != 0);
}

BOOL COSVersion::IsClusterServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_CLUSTER_SERVER) != 0);
}

BOOL COSVersion::IsStorageServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_STORAGE_SERVER) != 0);
}

BOOL COSVersion::IsEnterpriseStorageServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsStorageServer(lpVersionInformation) && ((lpVersionInformation->dwSuiteMask & COSVERSION_SUITE_ENTERPRISE) != 0);
}

BOOL COSVersion::IsExpressStorageServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STORAGE_EXPRESS_SERVER) != 0);
}

BOOL COSVersion::IsStandardStorageServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return IsStorageServer(lpVersionInformation) && ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STANDARD) != 0);
}

BOOL COSVersion::IsWorkgroupStorageServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STORAGE_WORKGROUP_SERVER) != 0);
}

BOOL COSVersion::IsEssentialsStorageServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STORAGE_SERVER_ESSENTIALS) != 0);
}

BOOL COSVersion::IsPreRelease(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_PRERELEASE) != 0);
}

BOOL COSVersion::IsEvaluation(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_EVALUATION) != 0);
}

BOOL COSVersion::IsMultipointServerPremiumEdition(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_MULTIPOINT_SERVER_PREMIUM) != 0);
}

BOOL COSVersion::IsPremium(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_PREMIUM) != 0);
}

BOOL COSVersion::IsThinPC(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_THINPC) != 0);
}

BOOL COSVersion::IsAutomotive(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_AUTOMOTIVE) != 0);
}

BOOL COSVersion::IsChina(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_CHINA) != 0);
}

BOOL COSVersion::IsSingleLanguage(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_SINGLE_LANGUAGE) != 0);
}

BOOL COSVersion::IsWin32sInstalled(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_WIN32S) != 0);
}

BOOL COSVersion::IsWindows8Point1Or2012R2Update(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_WINDOWS812012R2UPDATE) != 0);
}

BOOL COSVersion::IsCoreConnected(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_CORECONNECTED) != 0);
}

BOOL COSVersion::IsEducation(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_EDUCATION) != 0);
}

BOOL COSVersion::IsIndustry(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_INDUSTRY) != 0);
}

BOOL COSVersion::IsCore(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_CORE) != 0);
}

BOOL COSVersion::IsStudent(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_STUDENT) != 0);
}

BOOL COSVersion::IsMobile(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_MOBILE) != 0);
}

BOOL COSVersion::IsIoT(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_IOT) != 0);
}

BOOL COSVersion::IsCloudHostInfrastructureServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask3 & COSVERSION_SUITE3_CLOUD_HOST_INFRASTRUCTURE_SERVER) != 0);
}

BOOL COSVersion::IsSEdition(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_S) != 0);
}

BOOL COSVersion::IsNanoServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask2 & COSVERSION_SUITE2_NANO_SERVER) != 0);
}

BOOL COSVersion::IsCloudStorageServer(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask3 & COSVERSION_SUITE3_CLOUD_STORAGE_SERVER) != 0);
}

BOOL COSVersion::IsARM64Server(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask3 & COSVERSION_SUITE3_ARM64_SERVER) != 0);
}

BOOL COSVersion::IsPPIPro(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask3 & COSVERSION_SUITE3_PPI_PRO) != 0);
}

BOOL COSVersion::IsConnectedCar(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask3 & COSVERSION_SUITE3_CONNECTED_CAR) != 0);
}

BOOL COSVersion::IsHandheld(LPOS_VERSION_INFO lpVersionInformation)
{
  return ((lpVersionInformation->dwSuiteMask3 & COSVERSION_SUITE3_HANDHELD) != 0);
}

BOOL COSVersion::Is64Bit(LPOS_VERSION_INFO /*lpVersionInformation*/, BOOL bCheckUnderlying)
{
  if (bCheckUnderlying)
  {
  #if defined(COSVERSION_WIN64)
    return TRUE;
  #elif defined(COSVERSION_CE)
    return FALSE;
  #elif defined(COSVERSION_WIN32)
    //Assume the worst
    BOOL bSuccess = FALSE;

  #if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
    //Check to see if we are running on 64 bit Windows thro WOW64
    HMODULE hKernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
    if (hKernel32 != NULL)
    {
      lpfnIsWow64Process pIsWow64Process = (lpfnIsWow64Process) GetProcAddress(hKernel32, "IsWow64Process"); 
      if (pIsWow64Process)
      {
        BOOL bWow64 = FALSE;
        if (pIsWow64Process(GetCurrentProcess(), &bWow64))
          bSuccess = bWow64;
      }
    }
  #else
    //No way to determine if the underlying OS is 64 bit using WinRT
  #endif //#if (defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))

    return bSuccess;
  #else
    return FALSE;
  #endif //#if defined(COSVERSION_WIN64)
  }
  else
  {
  #if defined(COSVERSION_WIN64)
    return TRUE;
  #else
    return FALSE;
  #endif //#if defined(COSVERSION_WIN64)
  }
}

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#if defined(COSVERSION_DOS)
BOOL COSVersion::GetVersion(DWORD& /*dwVersion*/)
#else               
BOOL COSVersion::GetVersion(DWORD& dwVersion)
#endif
{
  //What will be the return value from this method (assume the worst)
  BOOL bSuccess = FALSE;

#if defined(COSVERSION_WIN16)
  if (m_lpfnGetVersion)
  {
    bSuccess = TRUE;
    dwVersion = m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, m_lpfnGetVersion, 0);
  }
#elif defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  //Get Kernel handle
  HMODULE hKernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
  if (hKernel32 == NULL)
    return FALSE;

  //Get the function pointer to "GetVersion"
#if defined(COSVERSION_CE)
  lpfnGetVersion lpGetVersion = (lpfnGetVersion)GetProcAddress(hKernel32, L"GetVersion");
#else
  lpfnGetVersion lpGetVersion = (lpfnGetVersion)GetProcAddress(hKernel32, "GetVersion");
#endif //#if defined(COSVERSION_CE)
  if (lpGetVersion == NULL)
    return FALSE;

  bSuccess = TRUE;
  dwVersion = lpGetVersion();
#endif //#if defined(COSVERSION_WIN16)

  return bSuccess;
}

BOOL COSVersion::GetVersionEx(LPOSVERSIONINFO lpVersionInfo)
{
  //Delegate to the other version
  return GetVersionEx((LPOSVERSIONINFOEX)lpVersionInfo);
}

#if defined(COSVERSION_DOS)
BOOL COSVersion::GetVersionEx(LPOSVERSIONINFOEX /*lpVersionInfo*/)
#else                  
BOOL COSVersion::GetVersionEx(LPOSVERSIONINFOEX lpVersionInfo)
#endif //#if defined(COSVERSION_DOS)
{
  //What will be the return value from this method (assume the worst)
  BOOL bSuccess = FALSE;

#if defined(COSVERSION_WIN16)
  if (m_lpfnGetVersionExA)
    bSuccess = (BOOL)m_lpfnCallProcEx32W(CPEX_DEST_STDCALL | 1, 0x01, m_lpfnGetVersionExA, lpVersionInfo, 0);
#elif defined(COSVERSION_WIN32) || defined(COSVERSION_WIN64)
  //Get Kernel handle
  HMODULE hKernel32 = GetModuleHandle(_T("KERNEL32.DLL"));
  if (hKernel32 == NULL)
    return FALSE;

  //Get the function pointer to "GetVersionEx"
#if defined(_UNICODE)
#if defined(COSVERSION_CE)
  lpfnGetVersionEx lpGetVersionEx = (lpfnGetVersionEx)GetProcAddress(hKernel32, L"GetVersionExW");
#else
  lpfnGetVersionEx lpGetVersionEx = (lpfnGetVersionEx)GetProcAddress(hKernel32, "GetVersionExW");
#endif //#if defined(COSVERSION_CE)
#else
  lpfnGetVersionEx lpGetVersionEx = (lpfnGetVersionEx)GetProcAddress(hKernel32, "GetVersionExA");
#endif //#if defined(_UNICODE)

  if (lpGetVersionEx)
    bSuccess = lpGetVersionEx((LPOSVERSIONINFO)lpVersionInfo);
#endif //#if defined(COSVERSION_WIN16)

  return bSuccess;
}
#endif //#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
