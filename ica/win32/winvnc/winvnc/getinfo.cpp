#include <windows.h> 
#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#endif
#include <memory.h>
//#include <math.h>
#include <stdio.h>    
#include <string.h>
#include "dtwinver.h"

#if !defined(_WIN32) && !defined(_WIN64)
  #define _stprintf sprintf
  #define _tcscat strcat
  #define LPTSTR LPSTR
#endif


int getinfo(char mytext[1024])

{                                  
  COSVersion::OS_VERSION_INFO osvi;
  memset(&osvi, 0, sizeof(osvi));
#ifdef _WIN32
  TCHAR sText[512];
  TCHAR sBuf[100];
#else
  char sText[512];
  char sBuf[100];
#endif

  COSVersion os;  
  if (os.GetVersion(&osvi))
  {
  #ifndef UNDER_CE
    _stprintf(sText, _T("OS: "));
    
    switch (osvi.EmulatedPlatform)
    {
      case COSVersion::Dos:               
      {
        _tcscat(sText, _T("DOS"));                
        break;
      }
      case COSVersion::Windows3x:         
      {
        _tcscat(sText, _T("Windows"));        
        if (os.IsWin32sInstalled(&osvi))
          _tcscat(sText, _T(" (Win32s)"));
        break;
      }
      case COSVersion::WindowsCE:
      {
        _tcscat(sText, _T("Windows CE"));        
        break;
      }
      case COSVersion::Windows9x:
      {
        if (os.IsWindows95(&osvi))
          _stprintf(sBuf, _T("Windows 95"));
        else if (os.IsWindows95SP1(&osvi))
          _stprintf(sBuf, _T("Windows 95 SP1"));
        else if (os.IsWindows95OSR2(&osvi))
          _stprintf(sBuf, _T("Windows 95 OSR2"));
        else if (os.IsWindows98(&osvi))
          _stprintf(sBuf, _T("Windows 98"));
        else if (os.IsWindows98SP1(&osvi))
          _stprintf(sBuf, _T("Windows 98 SP1"));
        else if (os.IsWindows98SE(&osvi))
          _stprintf(sBuf, _T("Windows 98 Second Edition"));
        else if (os.IsWindowsME(&osvi))
          _stprintf(sBuf, _T("Windows Millenium Edition"));
        else
          _stprintf(sBuf, _T("Windows ??"));
        _tcscat(sText, sBuf);          
        break;
      }
      case COSVersion::WindowsNT:
      {
        if (os.IsNTPreWin2k(&osvi))
        {
          _tcscat(sText, _T("Windows NT"));          

          if (os.IsNTWorkstation(&osvi))
            _tcscat(sText, _T(" (Workstation)"));
          else if (os.IsNTStandAloneServer(&osvi))
            _tcscat(sText, _T(" (Server)"));
          else if (os.IsNTPDC(&osvi))
            _tcscat(sText, _T(" (Primary Domain Controller)"));
          else if (os.IsNTBDC(&osvi))
            _tcscat(sText, _T(" (Backup Domain Controller)"));

          if (os.IsNTDatacenterServer(&osvi))
            _tcscat(sText, _T(", (Datacenter)"));
          else if (os.IsNTEnterpriseServer(&osvi))
            _tcscat(sText, _T(", (Enterprise)"));
        }
        else if (os.IsWindows2000(&osvi))
        {
          _tcscat(sText, _T("Windows 2000"));          

          if (os.IsWin2000Professional(&osvi))
            _tcscat(sText, _T(" (Professional)"));
          else if (os.IsWin2000Server(&osvi))
            _tcscat(sText, _T(" (Server)"));
          else if (os.IsWin2000DomainController(&osvi))
            _tcscat(sText, _T(" (Domain Controller)"));

          if (os.IsWin2000DatacenterServer(&osvi))
            _tcscat(sText, _T(", (Datacenter)"));
          else if (os.IsWin2000AdvancedServer(&osvi))
            _tcscat(sText, _T(", (Advanced Server)"));
        }
        else if (os.IsWindowsXPOrWindowsServer2003(&osvi))
        {
          if (os.IsXPPersonal(&osvi))
            _tcscat(sText, _T("Windows XP (Personal)"));          
          else if (os.IsXPProfessional(&osvi))
            _tcscat(sText, _T("Windows XP (Professional)"));          
          else if (os.IsWindowsServer2003(&osvi))
            _tcscat(sText, _T("Windows Server 2003"));          
          else if (os.IsDomainControllerWindowsServer2003(&osvi))
            _tcscat(sText, _T("Windows Server 2003 (Domain Controller)"));          

          if (os.IsDatacenterWindowsServer2003(&osvi))
            _tcscat(sText, _T(", (Datacenter Edition)"));
          else if (os.IsEnterpriseWindowsServer2003(&osvi))
            _tcscat(sText, _T(", (Enterprise Edition)"));
          else if (os.IsWebWindowsServer2003(&osvi))
            _tcscat(sText, _T(", (Web Edition)"));
          else if (os.IsWindowsServer2003(&osvi))
            _tcscat(sText, _T(", (Standard Edition)"));
        }

        if (os.IsTerminalServicesInstalled(&osvi))
          _tcscat(sText, _T(", (Terminal Services)"));
        if (os.ISSmallBusinessEditionInstalled(&osvi))
          _tcscat(sText, _T(", (BackOffice Small Business Edition)"));
        if (os.IsEmbedded(&osvi))
          _tcscat(sText, _T(", (Embedded)"));
        if (os.IsTerminalServicesInRemoteAdminMode(&osvi))
          _tcscat(sText, _T(", (Terminal Services in Remote Admin Mode)"));
        if (os.IsEmulated64Bit(&osvi))
          _tcscat(sText, _T(", (64 Bit Edition)"));
        if (os.IsMediaCenterInstalled(&osvi))
          _tcscat(sText, _T(", (Media Center Edition)"));

        if (osvi.dwSuiteMask & COSVERSION_SUITE_UNIPROCESSOR_FREE)
          _tcscat(sText, _T(", (Uniprocessor Free)"));
        else if (osvi.dwSuiteMask & COSVERSION_SUITE_UNIPROCESSOR_CHECKED)
          _tcscat(sText, _T(", (Uniprocessor Checked)"));
        else if (osvi.dwSuiteMask & COSVERSION_SUITE_MULTIPROCESSOR_FREE)
          _tcscat(sText, _T(", (Multiprocessor Free)"));
        else if (osvi.dwSuiteMask & COSVERSION_SUITE_MULTIPROCESSOR_CHECKED)
          _tcscat(sText, _T(", (Multiprocessor Checked)"));

        break;
      }
      default: 
      {
        _stprintf(sBuf, _T("Unknown OS"));
        break;
      }
    }                     

#ifndef UNDER_CE
    switch (osvi.EmulatedProcessorType)
    {
      case COSVersion::INTEL_PROCESSOR:
      {
        _tcscat(sText, _T(", (Intel Processor)"));
        break;
      }
      case COSVersion::MSIL_PROCESSOR:
      {
        _tcscat(sText, _T(", (MSIL Processor)"));
        break;
      }
      case COSVersion::MIPS_PROCESSOR:
      {
        _tcscat(sText, _T(", (MIPS Processor)"));
        break;
      }
      case COSVersion::ARM_PROCESSOR:
      {
        _tcscat(sText, _T(", (ARM Processor)"));
        break;
      }
      case COSVersion::SHX_PROCESSOR:
      {
        _tcscat(sText, _T(", (SHX Processor)"));
        break;
      }
      case COSVersion::ALPHA_PROCESSOR:
      {
        _tcscat(sText, _T(", (Alpha Processor)"));
        break;
      }
      case COSVersion::ALPHA64_PROCESSOR:
      {
        _tcscat(sText, _T(", (Alpha64 Processor)"));
        break;
      }
      case COSVersion::PPC_PROCESSOR:
      {
        _tcscat(sText, _T(", (PPC Processor)"));
        break;
      }
      case COSVersion::IA64_PROCESSOR:
      {
        _tcscat(sText, _T(", (IA64 Processor)"));
        break;
      }
      case COSVersion::AMD64_PROCESSOR:
      {
        _tcscat(sText, _T(", (AMD64 Processor)"));
        break;
      }
      case COSVersion::UNKNOWN_PROCESSOR: //deliberate fallthrough
      default:
      {
        _tcscat(sText, _T(", (Unknown Processor)"));
        break;
      }
    }
#endif

    _stprintf(sBuf, _T(" v%d."), osvi.dwEmulatedMajorVersion);
    _tcscat(sText, sBuf);     
    if (osvi.dwEmulatedMinorVersion % 10)
    {
      if (osvi.dwEmulatedMinorVersion > 9)
        _stprintf(sBuf, _T("%02d"), osvi.dwEmulatedMinorVersion);
      else
        _stprintf(sBuf, _T("%01d"), osvi.dwEmulatedMinorVersion);
    }
    else
      _stprintf(sBuf, _T("%01d"), osvi.dwEmulatedMinorVersion / 10);
    _tcscat(sText, sBuf);                           
    if (osvi.dwEmulatedBuildNumber)
    {
      _stprintf(sBuf, _T(" Build:%d"), osvi.dwEmulatedBuildNumber);
      _tcscat(sText, sBuf);           
    }
    if (osvi.wEmulatedServicePackMajor)       
    {
      if (osvi.wEmulatedServicePackMinor)
      {
        //Handle the special case of NT 4 SP 6a which Dtwinver ver treats as SP 6.1
        if (os.IsNTPreWin2k(&osvi) && (osvi.wEmulatedServicePackMajor == 6) && (osvi.wEmulatedServicePackMinor == 1))
          _stprintf(sBuf, _T(" Service Pack: 6a"));
        //Handle the special case of XP SP 1a which Dtwinver ver treats as SP 1.1
        else if (os.IsWindowsXP(&osvi) && (osvi.wEmulatedServicePackMajor == 1) && (osvi.wEmulatedServicePackMinor == 1))
          _stprintf(sBuf, _T(" Service Pack: 1a"));
        else       
          _stprintf(sBuf, _T(" Service Pack:%d.%d"), osvi.wEmulatedServicePackMajor, osvi.wEmulatedServicePackMinor);
      }
      else
        _stprintf(sBuf, _T(" Service Pack:%d"), osvi.wEmulatedServicePackMajor);
      _tcscat(sText, sBuf);
    }                            
    else
    {
      if (osvi.wEmulatedServicePackMinor)       
        _stprintf(sBuf, _T(" Service Pack:0.%d"), osvi.wEmulatedServicePackMinor);
    }

    _tcscat(sText, _T("\n"));        
  #endif
  /*  
    //CE does not really have a concept of an emulated OS so
    //lets not bother displaying any info on this on CE
    if (osvi.UnderlyingPlatform == COSVersion::WindowsCE)
      _tcscpy(sText, _T("OS: "));
    else
      _tcscat(sText, _T("Underlying OS: "));
                                                         
    switch (osvi.UnderlyingPlatform)
    {
      case COSVersion::Dos:               
      {
        _tcscat(sText, _T("DOS"));                
        break;
      }
      case COSVersion::Windows3x:         
      {
        _tcscat(sText, _T("Windows"));                
        break;
      }
      case COSVersion::WindowsCE:            
      {
        _tcscat(sText, _T("Windows CE"));                
        break;
      }
      case COSVersion::Windows9x:           
      {
        if (os.IsWindows95(&osvi))
          _stprintf(sBuf, _T("Windows 95"));
        else if (os.IsWindows95SP1(&osvi))
          _stprintf(sBuf, _T("Windows 95 SP1"));
        else if (os.IsWindows95OSR2(&osvi))
          _stprintf(sBuf, _T("Windows 95 OSR2"));
        else if (os.IsWindows98(&osvi))
          _stprintf(sBuf, _T("Windows 98"));
        else if (os.IsWindows98SP1(&osvi))
          _stprintf(sBuf, _T("Windows 98 SP1"));
        else if (os.IsWindows98SE(&osvi))
          _stprintf(sBuf, _T("Windows 98 Second Edition"));
        else if (os.IsWindowsME(&osvi))
          _stprintf(sBuf, _T("Windows Millenium Edition"));
        else
          _stprintf(sBuf, _T("Windows ??"));
        _tcscat(sText, sBuf);                  
        break;
      }
      case COSVersion::WindowsNT:    
      {
        if (os.IsNTPreWin2k(&osvi))
        {
          _tcscat(sText, _T("Windows NT"));                  

          if (os.IsNTWorkstation(&osvi))
            _tcscat(sText, _T(" (Workstation)"));
          else if (os.IsNTStandAloneServer(&osvi))
            _tcscat(sText, _T(" (Server)"));
          else if (os.IsNTPDC(&osvi))
            _tcscat(sText, _T(" (Primary Domain Controller)"));
          else if (os.IsNTBDC(&osvi))
            _tcscat(sText, _T(" (Backup Domain Controller)"));

          if (os.IsNTDatacenterServer(&osvi))
            _tcscat(sText, _T(", (Datacenter)"));
          else if (os.IsNTEnterpriseServer(&osvi))
            _tcscat(sText, _T(", (Enterprise)"));
        }
        else if (os.IsWindows2000(&osvi))
        {
          _tcscat(sText, _T("Windows 2000"));                  

          if (os.IsWin2000Professional(&osvi))
            _tcscat(sText, _T(" (Professional)"));
          else if (os.IsWin2000Server(&osvi))
            _tcscat(sText, _T(" (Server)"));
          else if (os.IsWin2000DomainController(&osvi))
            _tcscat(sText, _T(" (Domain Controller)"));

          if (os.IsWin2000DatacenterServer(&osvi))
            _tcscat(sText, _T(", (Datacenter)"));
          else if (os.IsWin2000AdvancedServer(&osvi))
            _tcscat(sText, _T(", (Advanced Server)"));
        }
        else if (os.IsWindowsXPOrWindowsServer2003(&osvi))
        {
          if (os.IsXPPersonal(&osvi))
            _tcscat(sText, _T("Windows XP (Personal)"));                  
          else if (os.IsXPProfessional(&osvi))
            _tcscat(sText, _T("Windows XP (Professional)"));                  
          else if (os.IsWindowsServer2003(&osvi))
            _tcscat(sText, _T("Windows Server 2003"));          
          else if (os.IsDomainControllerWindowsServer2003(&osvi))
            _tcscat(sText, _T("Windows Server 2003 (Domain Controller)"));          

          if (os.IsDatacenterWindowsServer2003(&osvi))
            _tcscat(sText, _T(", (Datacenter Edition)"));
          else if (os.IsEnterpriseWindowsServer2003(&osvi))
            _tcscat(sText, _T(", (Enterprise Edition)"));
          else if (os.IsWebWindowsServer2003(&osvi))
            _tcscat(sText, _T(", (Web Edition)"));
          else if (os.IsWindowsServer2003(&osvi))
            _tcscat(sText, _T(", (Standard Edition)"));
        }

        if (os.IsTerminalServicesInstalled(&osvi))
          _tcscat(sText, _T(", (Terminal Services)"));
        if (os.ISSmallBusinessEditionInstalled(&osvi))
          _tcscat(sText, _T(", (BackOffice Small Business Edition)"));
        if (os.IsEmbedded(&osvi))
          _tcscat(sText, _T(", (Embedded)"));
        if (os.IsTerminalServicesInRemoteAdminMode(&osvi))
          _tcscat(sText, _T(", (Terminal Services in Remote Admin Mode)"));
        if (os.IsEmulated64Bit(&osvi))
          _tcscat(sText, _T(", (64 Bit Edition)"));
        if (os.IsMediaCenterInstalled(&osvi))
          _tcscat(sText, _T(", (Media Center Edition)"));

        if (osvi.dwSuiteMask & COSVERSION_SUITE_UNIPROCESSOR_FREE)
          _tcscat(sText, _T(", (Uniprocessor Free)"));
        else if (osvi.dwSuiteMask & COSVERSION_SUITE_UNIPROCESSOR_CHECKED)
          _tcscat(sText, _T(", (Uniprocessor Checked)"));
        else if (osvi.dwSuiteMask & COSVERSION_SUITE_MULTIPROCESSOR_FREE)
          _tcscat(sText, _T(", (Multiprocessor Free)"));
        else if (osvi.dwSuiteMask & COSVERSION_SUITE_MULTIPROCESSOR_CHECKED)
          _tcscat(sText, _T(", (Multiprocessor Checked)"));

        break;
      }
      default:                         
      {
        _stprintf(sBuf, _T("Unknown OS"));
        _tcscat(sText, sBuf);                
        break;
      }
    }                                      

#ifndef UNDER_CE
    switch (osvi.UnderlyingProcessorType)
    {
      case COSVersion::INTEL_PROCESSOR:
      {
        _tcscat(sText, _T(", (Intel Processor)"));
        break;
      }
      case COSVersion::MSIL_PROCESSOR:
      {
        _tcscat(sText, _T(", (MSIL Processor)"));
        break;
      }
      case COSVersion::MIPS_PROCESSOR:
      {
        _tcscat(sText, _T(", (MIPS Processor)"));
        break;
      }
      case COSVersion::ARM_PROCESSOR:
      {
        _tcscat(sText, _T(", (ARM Processor)"));
        break;
      }
      case COSVersion::SHX_PROCESSOR:
      {
        _tcscat(sText, _T(", (SHX Processor)"));
        break;
      }
      case COSVersion::ALPHA_PROCESSOR:
      {
        _tcscat(sText, _T(", (Alpha Processor)"));
        break;
      }
      case COSVersion::PPC_PROCESSOR:
      {
        _tcscat(sText, _T(", (PPC Processor)"));
        break;
      }
      case COSVersion::IA64_PROCESSOR:
      {
        _tcscat(sText, _T(", (IA64 Processor)"));
        break;
      }
      case COSVersion::AMD64_PROCESSOR:
      {
        _tcscat(sText, _T(", (AMD64 Processor)"));
        break;
      }
      case COSVersion::UNKNOWN_PROCESSOR: //deliberate fallthrough
      default:
      {
        _tcscat(sText, _T(", (Unknown Processor)"));
        break;
      }
    }
#endif

    _stprintf(sBuf, _T(" v%d."), osvi.dwUnderlyingMajorVersion);
    _tcscat(sText, sBuf);     
    if (osvi.dwUnderlyingMinorVersion % 10)
    {
      if (osvi.dwUnderlyingMinorVersion > 9)
        _stprintf(sBuf, _T("%02d"), osvi.dwUnderlyingMinorVersion);
      else
        _stprintf(sBuf, _T("%01d"), osvi.dwUnderlyingMinorVersion);
    }
    else
      _stprintf(sBuf, _T("%01d"), osvi.dwUnderlyingMinorVersion / 10);
    _tcscat(sText, sBuf);          
    if (osvi.dwUnderlyingBuildNumber)
    {
      _stprintf(sBuf, _T(" Build:%d"), osvi.dwUnderlyingBuildNumber);
      _tcscat(sText, sBuf);
    }
    if (osvi.wUnderlyingServicePackMajor)       
    {
      if (osvi.wUnderlyingServicePackMinor)       
      {
        //Handle the special case of NT 4 SP 6a which Dtwinver ver treats as SP 6.1
        if (os.IsNTPreWin2k(&osvi) && (osvi.wUnderlyingServicePackMajor == 6) && (osvi.wUnderlyingServicePackMinor == 1))
          _stprintf(sBuf, _T(" Service Pack: 6a"));
        //Handle the special case of XP SP 1a which Dtwinver ver treats as SP 1.1
        else if (os.IsWindowsXP(&osvi) && (osvi.wUnderlyingServicePackMajor == 1) && (osvi.wUnderlyingServicePackMinor == 1))
          _stprintf(sBuf, _T(" Service Pack: 1a"));
        else
          _stprintf(sBuf, _T(" Service Pack:%d.%d"), osvi.wUnderlyingServicePackMajor, osvi.wUnderlyingServicePackMinor);
      }
      else
        _stprintf(sBuf, _T(" Service Pack:%d"), osvi.wUnderlyingServicePackMajor);
      _tcscat(sText, sBuf);
    }                            
    else
    {
      if (osvi.wUnderlyingServicePackMinor)       
        _stprintf(sBuf, _T(" Service Pack:0.%d"), osvi.wUnderlyingServicePackMinor);
    }
    _tcscat(sText, _T("\n"));    

    //Some extra info for CE
  #ifdef UNDER_CE
    if (osvi.UnderlyingPlatform == COSVersion::WindowsCE)
    {
      _tcscat(sText, _T("Model: "));
      _tcscat(sText, osvi.szOEMInfo);
      _tcscat(sText, _T("\nDevice Type: "));
      _tcscat(sText, osvi.szPlatformType);
    }
  #endif*/ 
  }
  else
    _stprintf(sText, _T("Failed in call to GetOSVersion\n"));
           
  

	char UserName[256 + 1]="";
	DWORD Size=256+1;
	if (GetUserName(UserName,&Size)!=0)
	{
		strcpy(mytext,"Current user : ");
		strcat(mytext,UserName);
	}
	char ComputerName[256+1]="";
	if (GetComputerName(ComputerName,&Size)!=0)
	{
		strcat(mytext,"\nComputerName : ");
		strcat(mytext,ComputerName);
	}
	///////////////////////////////

    char name[255];
	char *IP=NULL;
	PHOSTENT hostinfo;
		if(gethostname(name, sizeof(name))==0)
		{
			if((hostinfo=gethostbyname(name)) != NULL)
			{
				IP = inet_ntoa(*(struct in_addr*)* hostinfo->h_addr_list);
			}
		}
	if (IP)
	{
	strcat(mytext,"\nIP : ");
	strcat(mytext,IP);
	}

	//////////////////////////////////////
/*	MEMORYSTATUS memoryStatus;
	ZeroMemory(&memoryStatus,sizeof(MEMORYSTATUS));
	memoryStatus.dwLength = sizeof (MEMORYSTATUS);
	
	::GlobalMemoryStatus (&memoryStatus);
	
	sprintf("Installed RAM: %ldMB",(DWORD) ceil(memoryStatus.dwTotalPhys/1024/1024));	
	sprintf("\r\nMemory Available: %ldKB",(DWORD) (memoryStatus.dwAvailPhys/1024));	
	sprintf("\r\nPrecent of used RAM: %%%ld\n",memoryStatus.dwMemoryLoad);
	sprintf("\n");
	sprintf(sText);*/
	strcat(mytext,"\n");
	strcat(mytext,sText);      

  return 0;
}
