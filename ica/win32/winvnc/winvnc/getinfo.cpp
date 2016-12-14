/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#endif
#include <memory.h>
//#include <math.h>
#include <stdio.h>    
#include <string.h>
#include "Dtwinver.h"

#if !defined(_WIN32) && !defined(_WIN64)
  #define _stprintf sprintf
  #define _tcscat strcat
  #define LPTSTR LPSTR
#endif

extern BOOL G_ipv6_allowed;


int getinfo(char mytext[1024])

{                                  
	COSVersion::OS_VERSION_INFO osvi;
	memset(&osvi, 0, sizeof(osvi));
	TCHAR sText[2048];
	TCHAR sBuf[100];

	COSVersion os;
	if (os.GetVersion(&osvi))
	{
	_stprintf(sText, _T(""));
	//CE does not really have a concept of an emulated OS so
	//lets not bother displaying any info on this on CE
	if (osvi.UnderlyingPlatform == COSVersion::WindowsCE)
		_tcscpy(sText, _T("OS: "));
	else
		_tcscat(sText, _T("OS: "));

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
		if (os.IsWindowsEmbeddedCompact(&osvi, TRUE))
			_tcscat(sText, _T("Windows Embedded Compact"));
		else if (os.IsWindowsCENET(&osvi, TRUE))
			_tcscat(sText, _T("Windows CE .NET"));
		else
			_tcscat(sText, _T("Windows CE"));
		break;
	}
	case COSVersion::Windows9x:
	{
		if (os.IsWindows95(&osvi, TRUE))
			_stprintf(sBuf, _T("Windows 95"));
		else if (os.IsWindows95SP1(&osvi, TRUE))
			_stprintf(sBuf, _T("Windows 95 SP1"));
		else if (os.IsWindows95B(&osvi, TRUE))
			_stprintf(sBuf, _T("Windows 95 B [aka OSR2]"));
		else if (os.IsWindows95C(&osvi, TRUE))
			_stprintf(sBuf, _T("Windows 95 C [aka OSR2.5]"));
		else if (os.IsWindows98(&osvi, TRUE))
			_stprintf(sBuf, _T("Windows 98"));
		else if (os.IsWindows98SP1(&osvi, TRUE))
			_stprintf(sBuf, _T("Windows 98 SP1"));
		else if (os.IsWindows98SE(&osvi, TRUE))
			_stprintf(sBuf, _T("Windows 98 Second Edition"));
		else if (os.IsWindowsME(&osvi, TRUE))
			_stprintf(sBuf, _T("Windows Millenium Edition"));
		else
			_stprintf(sBuf, _T("Windows \?\?"));
		_tcscat(sText, sBuf);
		break;
	}
	case COSVersion::WindowsNT:
	{
		if (os.IsNTPreWin2k(&osvi, TRUE))
		{
			_tcscat(sText, _T("Windows NT"));

			if (os.IsNTWorkstation(&osvi, TRUE))
				_tcscat(sText, _T(" (Workstation)"));
			else if (os.IsNTStandAloneServer(&osvi, TRUE))
				_tcscat(sText, _T(" (Server)"));
			else if (os.IsNTPDC(&osvi, TRUE))
				_tcscat(sText, _T(" (Primary Domain Controller)"));
			else if (os.IsNTBDC(&osvi, TRUE))
				_tcscat(sText, _T(" (Backup Domain Controller)"));

			if (os.IsNTDatacenterServer(&osvi, TRUE))
				_tcscat(sText, _T(", (Datacenter)"));
			else if (os.IsNTEnterpriseServer(&osvi, TRUE))
				_tcscat(sText, _T(", (Enterprise)"));
		}
		else if (os.IsWindows2000(&osvi, TRUE))
		{
			if (os.IsProfessional(&osvi))
				_tcscat(sText, _T("Windows 2000 (Professional)"));
			else if (os.IsWindows2000Server(&osvi, TRUE))
				_tcscat(sText, _T("Windows 2000 Server"));
			else if (os.IsWindows2000DomainController(&osvi, TRUE))
				_tcscat(sText, _T("Windows 2000 (Domain Controller)"));
			else
				_tcscat(sText, _T("Windows 2000"));

			if (os.IsWindows2000DatacenterServer(&osvi, TRUE))
				_tcscat(sText, _T(", (Datacenter)"));
			else if (os.IsWindows2000AdvancedServer(&osvi, TRUE))
				_tcscat(sText, _T(", (Advanced Server)"));
		}
		else if (os.IsWindowsXPOrWindowsServer2003(&osvi, TRUE))
		{
			if (os.IsStarterEdition(&osvi))
				_tcscat(sText, _T("Windows XP (Starter Edition)"));
			else if (os.IsPersonal(&osvi))
				_tcscat(sText, _T("Windows XP (Personal)"));
			else if (os.IsProfessional(&osvi))
				_tcscat(sText, _T("Windows XP (Professional)"));
			else if (os.IsWindowsServer2003(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2003"));
			else if (os.IsDomainControllerWindowsServer2003(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2003 (Domain Controller)"));
			else if (os.IsWindowsServer2003R2(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2003 R2"));
			else if (os.IsDomainControllerWindowsServer2003R2(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2003 R2 (Domain Controller)"));
			else
				_tcscat(sText, _T("Windows XP"));

			if (os.IsDatacenterWindowsServer2003(&osvi, TRUE) || os.IsDatacenterWindowsServer2003R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Datacenter Edition)"));
			else if (os.IsEnterpriseWindowsServer2003(&osvi, TRUE) || os.IsEnterpriseWindowsServer2003(&osvi, TRUE))
				_tcscat(sText, _T(", (Enterprise Edition)"));
			else if (os.IsWebWindowsServer2003(&osvi, TRUE) || os.IsWebWindowsServer2003R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Web Edition)"));
			else if (os.IsStandardWindowsServer2003(&osvi, TRUE) || os.IsStandardWindowsServer2003R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Standard Edition)"));
		}
		else if (os.IsWindowsVistaOrWindowsServer2008(&osvi, TRUE))
		{
			if (os.IsStarterEdition(&osvi))
				_tcscat(sText, _T("Windows Vista (Starter Edition)"));
			else if (os.IsHomeBasic(&osvi))
				_tcscat(sText, _T("Windows Vista (Home Basic)"));
			else if (os.IsHomeBasicPremium(&osvi))
				_tcscat(sText, _T("Windows Vista (Home Premium)"));
			else if (os.IsBusiness(&osvi))
				_tcscat(sText, _T("Windows Vista (Business)"));
			else if (os.IsEnterprise(&osvi))
				_tcscat(sText, _T("Windows Vista (Enterprise)"));
			else if (os.IsUltimate(&osvi))
				_tcscat(sText, _T("Windows Vista (Ultimate)"));
			else if (os.IsWindowsServer2008(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2008"));
			else if (os.IsDomainControllerWindowsServer2008(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2008 (Domain Controller)"));
			else
				_tcscat(sText, _T("Windows Vista"));

			if (os.IsDatacenterWindowsServer2008(&osvi, TRUE))
				_tcscat(sText, _T(", (Datacenter Edition)"));
			else if (os.IsEnterpriseWindowsServer2008(&osvi, TRUE))
				_tcscat(sText, _T(", (Enterprise Edition)"));
			else if (os.IsWebWindowsServer2008(&osvi, TRUE))
				_tcscat(sText, _T(", (Web Edition)"));
			else if (os.IsStandardWindowsServer2008(&osvi, TRUE))
				_tcscat(sText, _T(", (Standard Edition)"));
		}
		else if (os.IsWindows7OrWindowsServer2008R2(&osvi, TRUE))
		{
			if (os.IsThinPC(&osvi))
				_tcscat(sText, _T("Windows 7 Thin PC"));
			else if (os.IsStarterEdition(&osvi))
				_tcscat(sText, _T("Windows 7 (Starter Edition)"));
			else if (os.IsHomeBasic(&osvi))
				_tcscat(sText, _T("Windows 7 (Home Basic)"));
			else if (os.IsHomeBasicPremium(&osvi))
				_tcscat(sText, _T("Windows 7 (Home Premium)"));
			else if (os.IsProfessional(&osvi))
				_tcscat(sText, _T("Windows 7 (Professional)"));
			else if (os.IsEnterprise(&osvi))
				_tcscat(sText, _T("Windows 7 (Enterprise)"));
			else if (os.IsUltimate(&osvi))
				_tcscat(sText, _T("Windows 7 (Ultimate)"));
			else if (os.IsWindowsServer2008R2(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2008 R2"));
			else if (os.IsDomainControllerWindowsServer2008R2(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2008 R2 (Domain Controller)"));
			else
				_tcscat(sText, _T("Windows 7"));

			if (os.IsDatacenterWindowsServer2008R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Datacenter Edition)"));
			else if (os.IsEnterpriseWindowsServer2008R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Enterprise Edition)"));
			else if (os.IsWebWindowsServer2008R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Web Edition)"));
			else if (os.IsStandardWindowsServer2008R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Standard Edition)"));
		}
		else if (os.IsWindows8OrWindowsServer2012(&osvi, TRUE))
		{
			if (os.IsThinPC(&osvi))
				_tcscat(sText, _T("Windows 8 Thin PC"));
			else if (os.IsWindowsRT(&osvi, TRUE))
				_tcscat(sText, _T("Windows 8 RT"));
			else if (os.IsStarterEdition(&osvi))
				_tcscat(sText, _T("Windows 8 (Starter Edition)"));
			else if (os.IsProfessional(&osvi))
				_tcscat(sText, _T("Windows 8 (Pro)"));
			else if (os.IsEnterprise(&osvi))
				_tcscat(sText, _T("Windows 8 (Enterprise)"));
			else if (os.IsWindowsServer2012(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2012"));
			else if (os.IsDomainControllerWindowsServer2012(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2012 (Domain Controller)"));
			else
				_tcscat(sText, _T("Windows 8"));

			if (os.IsDatacenterWindowsServer2012(&osvi, TRUE))
				_tcscat(sText, _T(", (Datacenter Edition)"));
			else if (os.IsEnterpriseWindowsServer2012(&osvi, TRUE))
				_tcscat(sText, _T(", (Enterprise Edition)"));
			else if (os.IsWebWindowsServer2012(&osvi, TRUE))
				_tcscat(sText, _T(", (Web Edition)"));
			else if (os.IsStandardWindowsServer2012(&osvi, TRUE))
				_tcscat(sText, _T(", (Standard Edition)"));
		}
		else if (os.IsWindows8Point1OrWindowsServer2012R2(&osvi, TRUE))
		{
			if (os.IsThinPC(&osvi))
				_tcscat(sText, _T("Windows 8.1 Thin PC"));
			else if (os.IsWindowsRT(&osvi, TRUE))
				_tcscat(sText, _T("Windows 8.1 RT"));
			else if (os.IsStarterEdition(&osvi))
				_tcscat(sText, _T("Windows 8.1 (Starter Edition)"));
			else if (os.IsProfessional(&osvi))
				_tcscat(sText, _T("Windows 8.1 (Pro)"));
			else if (os.IsEnterprise(&osvi))
				_tcscat(sText, _T("Windows 8.1 (Enterprise)"));
			else if (os.IsWindowsServer2012R2(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2012 R2"));
			else if (os.IsDomainControllerWindowsServer2012R2(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2012 R2 (Domain Controller)"));
			else
				_tcscat(sText, _T("Windows 8.1"));

			if (os.IsCoreConnected(&osvi))
				_tcscat(sText, _T(", (with Bing / CoreConnected)"));
			if (os.IsWindows8Point1Or2012R2Update(&osvi))
				_tcscat(sText, _T(", (Update)"));

			if (os.IsDatacenterWindowsServer2012R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Datacenter Edition)"));
			else if (os.IsEnterpriseWindowsServer2012R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Enterprise Edition)"));
			else if (os.IsWebWindowsServer2012R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Web Edition)"));
			else if (os.IsStandardWindowsServer2012R2(&osvi, TRUE))
				_tcscat(sText, _T(", (Standard Edition)"));
		}
		else if (os.IsWindows10OrWindowsServer2016(&osvi, TRUE))
		{
			if (os.IsThinPC(&osvi))
				_tcscat(sText, _T("Windows 10 Thin PC"));
			else if (os.IsWindowsRT(&osvi, TRUE))
				_tcscat(sText, _T("Windows 10 RT"));
			else if (os.IsStarterEdition(&osvi))
				_tcscat(sText, _T("Windows 10 (Starter Edition)"));
			else if (os.IsCore(&osvi))
				_tcscat(sText, _T("Windows 10 (Home)"));
			else if (os.IsProfessional(&osvi))
				_tcscat(sText, _T("Windows 10 (Pro)"));
			else if (os.IsEnterprise(&osvi))
				_tcscat(sText, _T("Windows 10 (Enterprise)"));
			else if (os.IsNanoServer(&osvi))
				_tcscat(sText, _T("Windows 10 Nano Server"));
			else if (os.IsARM64Server(&osvi))
				_tcscat(sText, _T("Windows 10 ARM64 Server"));
			else if (os.IsWindowsServer2016(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2016"));
			else if (os.IsDomainControllerWindowsServer2016(&osvi, TRUE))
				_tcscat(sText, _T("Windows Server 2016 (Domain Controller)"));
			else
				_tcscat(sText, _T("Windows 10"));

			if (os.IsCoreConnected(&osvi))
				_tcscat(sText, _T(", (with Bing / CoreConnected)"));

			if (os.IsDatacenterWindowsServer2016(&osvi, TRUE))
				_tcscat(sText, _T(", (Datacenter Edition)"));
			else if (os.IsEnterpriseWindowsServer2016(&osvi, TRUE))
				_tcscat(sText, _T(", (Enterprise Edition)"));
			else if (os.IsWebWindowsServer2016(&osvi, TRUE))
				_tcscat(sText, _T(", (Web Edition)"));
			else if (os.IsStandardWindowsServer2016(&osvi, TRUE))
				_tcscat(sText, _T(", (Standard Edition)"));
		}
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
	case COSVersion::IA32_PROCESSOR:
	{
		_tcscat(sText, _T(", (x86-32 Processor)"));
		break;
	}
	case COSVersion::MIPS_PROCESSOR:
	{
		_tcscat(sText, _T(", (MIPS Processor)"));
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
		_tcscat(sText, _T(", (IA64 Itanium[2] Processor)"));
		break;
	}
	case COSVersion::AMD64_PROCESSOR:
	{
		_tcscat(sText, _T(", (x86-64 Processor)"));
		break;
	}
	case COSVersion::ALPHA64_PROCESSOR:
	{
		_tcscat(sText, _T(", (Alpha64 Processor)"));
		break;
	}
	case COSVersion::MSIL_PROCESSOR:
	{
		_tcscat(sText, _T(", (MSIL Processor)"));
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
	case COSVersion::UNKNOWN_PROCESSOR: //deliberate fallthrough
	default:
	{
		_tcscat(sText, _T(", (Unknown Processor)"));
		break;
	}
	}
#endif
	_stprintf(sBuf, _T(" v%d."), (int)(osvi.dwUnderlyingMajorVersion));
	_tcscat(sText, sBuf);
	if (osvi.dwUnderlyingMinorVersion % 10)
	{
		if (osvi.dwUnderlyingMinorVersion > 9)
			_stprintf(sBuf, _T("%02d"), (int)(osvi.dwUnderlyingMinorVersion));
		else
			_stprintf(sBuf, _T("%01d"), (int)(osvi.dwUnderlyingMinorVersion));
	}
	else
		_stprintf(sBuf, _T("%01d"), (int)(osvi.dwUnderlyingMinorVersion / 10));
	_tcscat(sText, sBuf);
	if (osvi.dwUnderlyingBuildNumber)
	{
		_stprintf(sBuf, _T(" Build:%d"), (int)(osvi.dwUnderlyingBuildNumber));
		_tcscat(sText, sBuf);
	}
	_tcscat(sText, _T("\n"));
	if (osvi.wUnderlyingServicePackMajor)
	{
		if (osvi.wUnderlyingServicePackMinor)
		{
			//Handle the special case of NT 4 SP 6a which Dtwinver treats as SP 6.1
			if (os.IsNTPreWin2k(&osvi, TRUE) && (osvi.wUnderlyingServicePackMajor == 6) && (osvi.wUnderlyingServicePackMinor == 1))
				_stprintf(sBuf, _T("Service Pack: 6a"));
			//Handle the special case of XP SP 1a which Dtwinver treats as SP 1.1
			else if (os.IsWindowsXP(&osvi, TRUE) && (osvi.wUnderlyingServicePackMajor == 1) && (osvi.wUnderlyingServicePackMinor == 1))
				_stprintf(sBuf, _T("Service Pack: 1a"));
			else
				_stprintf(sBuf, _T("Service Pack:%d.%d"), (int)(osvi.wUnderlyingServicePackMajor), (int)(osvi.wUnderlyingServicePackMinor));
		}
		else
			_stprintf(sBuf, _T("Service Pack:%d"), (int)(osvi.wUnderlyingServicePackMajor));
		_tcscat(sText, sBuf);
	}
	else
	{
		if (osvi.wUnderlyingServicePackMinor)
			_stprintf(sBuf, _T("Service Pack:0.%d"), (int)(osvi.wUnderlyingServicePackMinor));
	}
	if (os.IsEnterpriseStorageServer(&osvi))
		_tcscat(sText, _T(", (Storage Server Enterprise)"));
	else if (os.IsExpressStorageServer(&osvi))
		_tcscat(sText, _T(", (Storage Server Express)"));
	else if (os.IsStandardStorageServer(&osvi))
		_tcscat(sText, _T(", (Storage Server Standard)"));
	else if (os.IsWorkgroupStorageServer(&osvi))
		_tcscat(sText, _T(", (Storage Server Workgroup)"));
	else if (os.IsEssentialsStorageServer(&osvi))
		_tcscat(sText, _T(", (Storage Server Essentials)"));
	else if (os.IsStorageServer(&osvi))
		_tcscat(sText, _T(", (Storage Server)"));

	if (os.IsHomeServerPremiumEdition(&osvi))
		_tcscat(sText, _T(", (Home Server Premium Edition)"));
	else if (os.IsHomeServerEdition(&osvi))
		_tcscat(sText, _T(", (Home Server Edition)"));

	if (os.IsTerminalServices(&osvi))
		_tcscat(sText, _T(", (Terminal Services)"));
	if (os.IsEmbedded(&osvi))
		_tcscat(sText, _T(", (Embedded)"));
	if (os.IsTerminalServicesInRemoteAdminMode(&osvi))
		_tcscat(sText, _T(", (Terminal Services in Remote Admin Mode)"));
	if (os.Is64Bit(&osvi, TRUE))
		_tcscat(sText, _T(", (64 Bit Edition)"));
	if (os.IsMediaCenter(&osvi))
		_tcscat(sText, _T(", (Media Center Edition)"));
	if (os.IsTabletPC(&osvi))
		_tcscat(sText, _T(", (Tablet PC Edition)"));
	if (os.IsComputeClusterServerEdition(&osvi))
		_tcscat(sText, _T(", (Compute Cluster Edition)"));
	if (os.IsServerFoundation(&osvi))
		_tcscat(sText, _T(", (Foundation Edition)"));
	if (os.IsMultipointServerPremiumEdition(&osvi))
		_tcscat(sText, _T(", (MultiPoint Premium Edition)"));
	else if (os.IsMultiPointServer(&osvi))
		_tcscat(sText, _T(", (MultiPoint Edition)"));
	if (os.IsSecurityAppliance(&osvi))
		_tcscat(sText, _T(", (Security Appliance)"));
	if (os.IsBackOffice(&osvi))
		_tcscat(sText, _T(", (BackOffice)"));
	if (os.IsNEdition(&osvi))
		_tcscat(sText, _T(", (N Edition)"));
	if (os.IsEEdition(&osvi))
		_tcscat(sText, _T(", (E Edition)"));
	if (os.IsHyperVTools(&osvi))
		_tcscat(sText, _T(", (Hyper-V Tools)"));
	if (os.IsHyperVServer(&osvi))
		_tcscat(sText, _T(", (Hyper-V Server)"));
	if (os.IsServerCore(&osvi))
		_tcscat(sText, _T(", (Server Core)"));
	if (os.IsUniprocessorFree(&osvi))
		_tcscat(sText, _T(", (Uniprocessor Free)"));
	if (os.IsUniprocessorChecked(&osvi))
		_tcscat(sText, _T(", (Uniprocessor Checked)"));
	if (os.IsMultiprocessorFree(&osvi))
		_tcscat(sText, _T(", (Multiprocessor Free)"));
	if (os.IsMultiprocessorChecked(&osvi))
		_tcscat(sText, _T(", (Multiprocessor Checked)"));
	if (os.IsEssentialBusinessServerManagement(&osvi))
		_tcscat(sText, _T(", (Windows Essential Business Server Manangement Server)"));
	if (os.IsEssentialBusinessServerMessaging(&osvi))
		_tcscat(sText, _T(", (Windows Essential Business Server Messaging Server)"));
	if (os.IsEssentialBusinessServerSecurity(&osvi))
		_tcscat(sText, _T(", (Windows Essential Business Server Security Server)"));
	if (os.IsClusterServer(&osvi))
		_tcscat(sText, _T(", (Cluster Server)"));
	if (os.IsSmallBusinessServer(&osvi))
		_tcscat(sText, _T(", (Small Business Server)"));
	if (os.IsSmallBusinessServerPremium(&osvi))
		_tcscat(sText, _T(", (Small Business Server Premium)"));
	if (os.IsPreRelease(&osvi))
		_tcscat(sText, _T(", (Prerelease)"));
	if (os.IsEvaluation(&osvi))
		_tcscat(sText, _T(", (Evaluation)"));
	if (os.IsAutomotive(&osvi))
		_tcscat(sText, _T(", (Automotive)"));
	if (os.IsChina(&osvi))
		_tcscat(sText, _T(", (China)"));
	if (os.IsSingleLanguage(&osvi))
		_tcscat(sText, _T(", (Single Language)"));
	if (os.IsWin32sInstalled(&osvi))
		_tcscat(sText, _T(", (Win32s)"));
	if (os.IsEducation(&osvi))
		_tcscat(sText, _T(", (Education)"));
	if (os.IsIndustry(&osvi))
		_tcscat(sText, _T(", (Industry)"));
	if (os.IsStudent(&osvi))
		_tcscat(sText, _T(", (Student)"));
	if (os.IsMobile(&osvi))
		_tcscat(sText, _T(", (Mobile)"));
	if (os.IsIoT(&osvi))
		_tcscat(sText, _T(", (IoT Core)"));
	if (os.IsCloudHostInfrastructureServer(&osvi))
		_tcscat(sText, _T(", (Cloud Host Infrastructure Server)"));
	if (os.IsSEdition(&osvi))
		_tcscat(sText, _T(", (S Edition)"));
	if (os.IsCloudStorageServer(&osvi))
		_tcscat(sText, _T(", (Cloud Storage Server)"));
	if (os.IsPPIPro(&osvi))
		_tcscat(sText, _T(", (PPI Pro)"));
	if (os.IsConnectedCar(&osvi))
		_tcscat(sText, _T(", (Connected Car)"));
	if (os.IsHandheld(&osvi))
		_tcscat(sText, _T(", (Handheld)"));

	_tcscat(sText, _T("\n"));
	 }
  else
	  _stprintf(sText, _T("Failed in call to GetOSVersion\n"));
           
  

	/*char UserName[256+1]="";
	DWORD Size=256+1;
	if (GetUserName(UserName,&Size)!=0)
	{
		strcpy(mytext,"Current user : ");
		strcat(mytext,UserName);
	}*/
	DWORD Size = 256 + 1;
	strcpy(mytext, "");
	char ComputerName[256+1]="";
	if (GetComputerName(ComputerName,&Size)!=0)
	{
		strcat(mytext,"\nComputerName : ");
		strcat(mytext,ComputerName);
	}
	///////////////////////////////

    char name[255];
	char *IP=NULL;
#ifdef IPV6V4

	LPSOCKADDR sockaddr_ip;
	char ipstringbuffer[46];
	DWORD ipbufferlength = 46;
	bool IsIpv4 = false;
	bool IsIpv6 = false;
	struct addrinfo hint;
	struct addrinfo *serverinfo = 0;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	struct sockaddr_in6 *pIpv6Addr;
	struct sockaddr_in *pIpv4Addr;
	struct sockaddr_in6 Ipv6Addr;
	struct sockaddr_in Ipv4Addr;
	memset(&Ipv6Addr, 0, sizeof(Ipv6Addr));
	memset(&Ipv4Addr, 0, sizeof(Ipv4Addr));

	if (getaddrinfo(name, 0, &hint, &serverinfo) == 0)
	{
		struct addrinfo *p;
		for (p = serverinfo; p != NULL; p = p->ai_next) {
			switch (p->ai_family) {
			case AF_INET:
				IsIpv4 = true;
				pIpv4Addr = (struct sockaddr_in *) p->ai_addr;
				memcpy(&Ipv4Addr, pIpv4Addr, sizeof(Ipv4Addr));
				Ipv4Addr.sin_family = AF_INET;
				break;
			case AF_INET6:
				IsIpv6 = true;
				pIpv6Addr = (struct sockaddr_in6 *) p->ai_addr;
				memcpy(&Ipv6Addr, pIpv6Addr, sizeof(Ipv6Addr));
				Ipv6Addr.sin6_family = AF_INET6;
				sockaddr_ip = (LPSOCKADDR)p->ai_addr;
				ipbufferlength = 46;
				memset(ipstringbuffer, 0, 46);
				WSAAddressToString(sockaddr_ip, (DWORD)p->ai_addrlen, NULL, ipstringbuffer, &ipbufferlength);
				break;
			default:
				break;
				}


			}

		}
	freeaddrinfo(serverinfo);

	if (IsIpv6 && IsIpv4)
	{
		char			szText[256];
		sprintf(szText, "Ipv4: %s\nIpv6: %s \n", inet_ntoa(Ipv4Addr.sin_addr), ipstringbuffer);
		strcat(mytext, szText);
	}
	else if (IsIpv6)
	{
		char			szText[256];
		sprintf(szText, "Ipv6: %s \n", ipstringbuffer);
		strcat(mytext, szText);
	}
	else if (IsIpv4)
	{
		char			szText[256];
		sprintf(szText, "Ipv4: %s \n", inet_ntoa(Ipv4Addr.sin_addr));
		strcat(mytext, szText);
	}
#else
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
#endif
	//////////////////////////////////////
	strcat(mytext,"\n");
	strcat(mytext,sText);      

  return 0;
}
