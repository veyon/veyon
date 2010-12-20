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

#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <windows.h>
#include "CpuUsage.h"

DWORD CProcessorUsage::s_TickMark = 0;
__int64  CProcessorUsage::s_time = 0;
__int64 CProcessorUsage::s_idleTime = 0;
__int64 CProcessorUsage::s_kernelTime = 0;
__int64 CProcessorUsage::s_userTime = 0;
__int64 CProcessorUsage::s_kernelTimeProcess = 0;
__int64 CProcessorUsage::s_userTimeProcess = 0;
int CProcessorUsage::s_count = 0;
int CProcessorUsage::s_index = 0;
int CProcessorUsage::s_lastCpu = 0;
int CProcessorUsage::s_cpu[5] = {0, 0, 0, 0, 0};
int CProcessorUsage::s_cpuProcess[5] = {0, 0, 0, 0, 0};

CProcessorUsage::CProcessorUsage()
{
   ::InitializeCriticalSection(&m_cs);
	m_bLocked = false;
   s_pfnNtQuerySystemInformation = NULL;
   s_pfnGetSystemTimes = NULL;

   m_pInfo = NULL;
   m_uInfoLength = sizeof(PROC_PERF_INFO);

   HMODULE hModule  = ::GetModuleHandle(_T("kernel32.dll"));
   if(hModule)
      s_pfnGetSystemTimes = (pfnGetSystemTimes)::GetProcAddress(hModule, "GetSystemTimes");

   if(!s_pfnGetSystemTimes)
   {
      hModule = ::GetModuleHandle(_T("ntdll.dll"));
      if(hModule)
      {
         s_pfnNtQuerySystemInformation = (pfnNtQuerySystemInformation)::GetProcAddress(hModule, "NtQuerySystemInformation");
         if(s_pfnNtQuerySystemInformation)
         {
            PROC_PERF_INFO tmp;
            s_pfnNtQuerySystemInformation(8, &tmp, m_uInfoLength, &m_uInfoLength);
            m_pInfo = new PROC_PERF_INFO[m_uInfoLength / sizeof(PROC_PERF_INFO)];
         }
      }
   }
   s_TickMark = ::GetTickCount();
}

CProcessorUsage::~CProcessorUsage()
{
   if(m_pInfo)
      delete m_pInfo;
     //Critical Section Over-Released or Corrupted
	//::LeaveCriticalSection( &m_cs );
	if (m_bLocked) m_bLocked = false;
    ::DeleteCriticalSection(&m_cs);
}

void CProcessorUsage::GetSysTimes(__int64 & idleTime, __int64 & kernelTime, __int64 & userTime)
{
   if(s_pfnGetSystemTimes)
      s_pfnGetSystemTimes((LPFILETIME)&idleTime, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime);
   else
   {
      idleTime = 0;
      kernelTime = 0;
      userTime = 0;
      if(s_pfnNtQuerySystemInformation && m_uInfoLength && !s_pfnNtQuerySystemInformation(0x08, m_pInfo, m_uInfoLength, &m_uInfoLength))
      {
         // NtQuerySystemInformation returns information for all
         // CPU cores in the system, so we take the average here:
         int nCores = m_uInfoLength / sizeof(PROC_PERF_INFO);
         for(int i = 0;i < nCores;i ++)
         {
            idleTime += m_pInfo[i].IdleTime.QuadPart;
            kernelTime += m_pInfo[i].KernelTime.QuadPart;
            userTime += m_pInfo[i].UserTime.QuadPart;
         }
         idleTime /= nCores;
         kernelTime /= nCores;
         userTime /= nCores;
      }
   }
}

short CProcessorUsage::GetUsage()
{
    __int64 sTime;
    int sLastCpu;

   if (m_bLocked) return s_lastCpu;
   ::EnterCriticalSection( &m_cs );
   m_bLocked = true;
   sTime = s_time;
   sLastCpu = s_lastCpu;

    if(((::GetTickCount() - s_TickMark) & 0x7FFFFFFF) <= 200)
	{
	  if (m_bLocked) ::LeaveCriticalSection( &m_cs );
	  m_bLocked = false;
      return sLastCpu;
	}

   __int64 time;
   __int64 idleTime;
   __int64 kernelTime;
   __int64 userTime;
   __int64 kernelTimeProcess;
   __int64 userTimeProcess;

   ::GetSystemTimeAsFileTime((LPFILETIME)&time);

    if(!sTime)
    {
      GetSysTimes(idleTime, kernelTime, userTime);
      FILETIME createTime;
      FILETIME exitTime;
      ::GetProcessTimes(::GetCurrentProcess(), &createTime, &exitTime, (LPFILETIME)&kernelTimeProcess, (LPFILETIME)&userTimeProcess);

      s_time = time;
      s_idleTime = idleTime;
      s_kernelTime = kernelTime;
      s_userTime = userTime;
      s_kernelTimeProcess = kernelTimeProcess;
      s_userTimeProcess = userTimeProcess;
      s_lastCpu = 0;
      s_TickMark = ::GetTickCount();
	  if (m_bLocked) ::LeaveCriticalSection( &m_cs );
	  m_bLocked = false;
      return 0;
   }

    __int64 div = (time - sTime);

   GetSysTimes(idleTime, kernelTime, userTime);

   FILETIME createTime;
   FILETIME exitTime;
   ::GetProcessTimes(::GetCurrentProcess(), &createTime, &exitTime, (LPFILETIME)&kernelTimeProcess, (LPFILETIME)&userTimeProcess);

    int cpu;
    int cpuProcess;

     __int64 usr = userTime   - s_userTime;
     __int64 ker = kernelTime - s_kernelTime;
     __int64 idl = idleTime   - s_idleTime;
     __int64 sys = (usr + ker);

   if(sys)
      cpu = int((sys - idl) * 100 / sys); // System Idle take 100 % of cpu;
   else
      cpu = 0;

   cpuProcess = int((((userTimeProcess - s_userTimeProcess) + (kernelTimeProcess - s_kernelTimeProcess)) * 100 ) / div);

   s_time = time;
   s_idleTime = idleTime;
   s_kernelTime = kernelTime;
   s_userTime = userTime;
   s_kernelTimeProcess = kernelTimeProcess;
   s_userTimeProcess = userTimeProcess;
   s_cpu[s_index] = cpu;
	s_cpuProcess[s_index] = cpuProcess;
	s_index++;
	s_index%=5;

   s_count ++;

   if(s_count > 5)
      s_count = 5;

   cpu = 0;
   int i;
   for(i = 0; i < s_count; i++ )
      cpu += s_cpu[i];

   cpuProcess = 0;
   for(i = 0; i < s_count; i++ )
      cpuProcess += s_cpuProcess[i];

   cpu /= s_count;
   cpuProcess /= s_count;
   s_lastCpu = cpuProcess;
   sLastCpu = s_lastCpu;
   s_TickMark = ::GetTickCount();
   if (m_bLocked) ::LeaveCriticalSection( &m_cs );
   m_bLocked = false;
   return sLastCpu;
}
