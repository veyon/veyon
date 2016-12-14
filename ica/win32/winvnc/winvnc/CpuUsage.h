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

#pragma once


////////////////////////////////////////
// class CProcessorUsage;
//
// Calculates overal processor usage at
// any given time.
//
// The usage value is updated every 200
// milliseconds;
//
// The class is fully thread-safe;
//
class CProcessorUsage
{
   typedef BOOL (WINAPI * pfnGetSystemTimes)(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime );
   typedef LONG (WINAPI * pfnNtQuerySystemInformation)(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

   struct PROC_PERF_INFO
   {
      LARGE_INTEGER IdleTime;
      LARGE_INTEGER KernelTime;
      LARGE_INTEGER UserTime;
      LARGE_INTEGER Reserved1[2];
      ULONG Reserved2;
   };

public:

    CProcessorUsage();
    ~CProcessorUsage();

    short GetUsage();
	bool m_bLocked;

private:

   void GetSysTimes(__int64 & idleTime, __int64 & kernelTime, __int64 & userTime);

   ////////////////////////////////////////////////
   // Set of static variables to be accessed from
   // within critical section by multiple threads:
   //
   static DWORD s_TickMark;
   static __int64 s_time;
   static __int64 s_idleTime;
   static __int64 s_kernelTime;
   static __int64 s_userTime;
   static int s_lastCpu;
   static int s_cpu[5];
   static __int64 s_kernelTimeProcess;
   static __int64 s_userTimeProcess;
   static int s_cpuProcess[5];
    static int s_count;
    static int s_index;
   //
   /////////////////////////////////////////////////

   pfnGetSystemTimes s_pfnGetSystemTimes;
   pfnNtQuerySystemInformation s_pfnNtQuerySystemInformation;
   CRITICAL_SECTION m_cs;
   PROC_PERF_INFO * m_pInfo;
   ULONG m_uInfoLength;
};
