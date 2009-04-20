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
