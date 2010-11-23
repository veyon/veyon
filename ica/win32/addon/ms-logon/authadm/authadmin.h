
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTHLOGONUSER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTHLOGONUSER_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef AUTHADMIN_EXPORTS
#define AUTHADMIN_API __declspec(dllexport)
#else
#define AUTHADMIN_API __declspec(dllimport)
#endif
#pragma comment( lib, "netapi32.lib" )
#if defined( UNICODE ) || defined( _UNICODE )
#error Sorry -- please compile as an ANSI program.
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define MAXLEN 256

AUTHADMIN_API
BOOL CUGP(char * userin,char *password,char *machine,char * group,int locdom);
