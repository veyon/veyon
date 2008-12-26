#ifndef __CPU_H__
#define __CPU_H__
#define CPUID_STD_FPU          0x00000001
#define CPUID_STD_VME          0x00000002
#define CPUID_STD_DEBUGEXT     0x00000004
#define CPUID_STD_4MPAGE       0x00000008
#define CPUID_STD_TSC          0x00000010
#define CPUID_STD_MSR          0x00000020
#define CPUID_STD_PAE          0x00000040
#define CPUID_STD_MCHKXCP      0x00000080
#define CPUID_STD_CMPXCHG8B    0x00000100
#define CPUID_STD_APIC         0x00000200
#define CPUID_STD_SYSENTER     0x00000800
#define CPUID_STD_MTRR         0x00001000
#define CPUID_STD_GPE          0x00002000
#define CPUID_STD_MCHKARCH     0x00004000
#define CPUID_STD_CMOV         0x00008000
#define CPUID_STD_PAT          0x00010000
#define CPUID_STD_PSE36        0x00020000
#define CPUID_STD_MMX          0x00800000
#define CPUID_STD_FXSAVE       0x01000000
#define CPUID_STD_SSE          0x02000000
#define CPUID_STD_SSE2         0x04000000
#define CPUID_EXT_3DNOW        0x80000000
#define CPUID_EXT_AMD_3DNOWEXT 0x40000000
#define CPUID_EXT_AMD_MMXEXT   0x00400000

#define FEATURE_CPUID           0x00000001
#define FEATURE_STD_FEATURES    0x00000002
#define FEATURE_EXT_FEATURES    0x00000004
#define FEATURE_TSC             0x00000010
#define FEATURE_MMX             0x00000020
#define FEATURE_CMOV            0x00000040
#define FEATURE_3DNOW           0x00000080
#define FEATURE_3DNOWEXT        0x00000100
#define FEATURE_MMXEXT          0x00000200
#define FEATURE_SSEFP           0x00000400
#define FEATURE_K6_MTRR         0x00000800
#define FEATURE_P6_MTRR         0x00001000
#define FEATURE_SSE				0x00002000
#define FEATURE_SSE2            0x00004000

class Ultravncmemcpy
{
public:
	Ultravncmemcpy();
	~Ultravncmemcpy();
	inline void Set_memcpu();
	bool Save_memcpy(void* dest,void* src,size_t count);
	bool Save_memcmp(void* dest,void* src,size_t count);
	inline void memcpyMMX(void* dest,void* src,DWORD count);
	inline void memcpySSE(void *dest, const void *src, size_t nbytes);
	UINT get_feature_flags(void);
	bool cputype;
};

#endif
