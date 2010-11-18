#ifdef IPP
#include "ippcore.h"
#include "ippversion.h"
#include <windows.h>

#define CACHE_LINE 32
#define bool BOOL
#define false FALSE
#define true TRUE


static int ippstaticinitcalled=0;
#if IPP_VERSION_MAJOR>=5
#define ippCoreGetStatusString ippGetStatusString
#define ippCoreGetCpuType ippGetCpuType
#define ippStaticInitBest ippStaticInit
#endif


// MMX_SSESupport.h

#pragma once

#define _MMX_FEATURE_BIT        0x00800000      // bit 23
#define _SSE_FEATURE_BIT        0x02000000      // bit 25
// bit flags set by cpuid when called with register eax set to 1
#define MMX_SUPPORTED			0x00800000
#define SSE_SUPPORTED			0x02000000
#define SSE2_SUPPORTED			0x04000000
#define AMD_3DNOW_SUPPORTED		0x80000000

// AMD specific
#define AMD_3DNOW_EX_SUPPORTED	0x40000000
#define AMD_MMX_EX_SUPPORTED	0x00400000

#define SUPPORT_MMX				0x0001
#define SUPPORT_3DNOW			0x0002
#define SUPPORT_SSE				0x0004
#define SUPPORT_SSE2			0x0008
DWORD				dwFeatures;


#ifndef _X64
bool IsCPUID()
{
	__try 
	{
		_asm 
		{
			xor eax, eax
			cpuid
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
		return false;
	}
	return true;
}

bool QueryCPUInfo()
{
	DWORD dwStandard = 0;
	DWORD dwFeature = 0;
	
	DWORD dwMax = 0;
	DWORD dwExt = 0;
	int feature = 0;
	int os_support = 0;

	union {
		char cBuffer[12+1];
		struct {
			DWORD dw0;
			DWORD dw1;
			DWORD dw2;
		} stc;
	} Vendor;

	if (!IsCPUID())
		return false;

	dwFeatures=0;
	memset(&Vendor, 0, sizeof(Vendor));

	_asm {
		push ebx
		push ecx
		push edx

		// get the vendor string
		xor eax, eax
		cpuid
		mov dwMax, eax
		mov Vendor.stc.dw0, ebx
		mov Vendor.stc.dw1, edx
		mov Vendor.stc.dw2, ecx

		// get the Standard bits
		mov eax, 1
		cpuid
		mov dwStandard, eax
		mov dwFeature, edx

		// get AMD-specials
		mov eax, 80000000h
		cpuid
		cmp eax, 80000000h
		jc notamd
		mov eax, 80000001h
		cpuid
		mov dwExt, edx

notamd:
		pop ecx
		pop ebx
		pop edx
	}

	if (dwFeature & MMX_SUPPORTED) 
		dwFeatures |= SUPPORT_MMX;

	if (dwExt & AMD_3DNOW_SUPPORTED) 
		dwFeatures |= SUPPORT_3DNOW;

	if (dwFeature & SSE_SUPPORTED) 
		dwFeatures |= SUPPORT_SSE;

	if (dwFeature & SSE2_SUPPORTED) 
		dwFeatures |= SUPPORT_SSE2;

	return true;
}


static
IppCpuType
AutoDetect(void)
{
IppCpuType cputype=ippCpuUnknown;
QueryCPUInfo();
if ((dwFeatures & SUPPORT_MMX) == SUPPORT_MMX) cputype=ippCpuPII;
if ((dwFeatures & SUPPORT_SSE) == SUPPORT_SSE) cputype=ippCpuPIII;
if ((dwFeatures & SUPPORT_SSE2) == SUPPORT_SSE2) cputype=ippCpuP4;
return cputype;
}
#endif

void InitIpp()
{
if(!ippstaticinitcalled)
	{
		IppCpuType cpu=ippCoreGetCpuType();
#ifndef _X64
		if(cpu==ippCpuUnknown) {cpu=AutoDetect();  ippInitCpu(cpu);}
		else
#endif
		ippStaticInitBest();
		ippstaticinitcalled=1;
	}
}
#endif