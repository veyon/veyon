
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTH_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTH_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef LDAPAUTH_EXPORTS
#define LDAPAUTH_API __declspec(dllexport)
#else
#define LDAPAUTH_API __declspec(dllimport)
#endif

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>
#include <math.h>
#include <objbase.h>
#include <activeds.h>
#include <Dsgetdc.h>
#include <sddl.h>
#include <assert.h>


#define MAXLEN 256
//#define MAX_PREFERRED_LENGTH    ((DWORD) -1)
//#define NERR_Success            0
//#define LG_INCLUDE_INDIRECT 1
#define BUFSIZE 1024

#define FETCH_NUM 100
void PrintBanner(LPOLESTR pwszBanner);
HRESULT GetObjectGuid(IDirectoryObject * pDO,BSTR &bsGuid);
BOOL RecursiveIsMember(IADsGroup * pADsGroup,LPWSTR pwszMemberGUID,LPWSTR pwszMemberPath, 
                                             BOOL bVerbose, LPOLESTR  pwszUser, LPOLESTR pwszPassword);

HRESULT FindUserByName(IDirectorySearch *pSearchBase, //Container to search
					   LPOLESTR szFindUser, //Name of user to find.
					   IADs **ppUser); //Return a pointer to the user


HRESULT FindGroup(IDirectorySearch *pSearchBase, //Container to search
					   LPOLESTR szFindUser, //Name of user to find.
					   IADs **ppUser,LPOLESTR szGroup); //Return a pointer to the user

BSTR gbsGroup     = NULL; // <Group to check> This is the LDAP path for the group to check
	BSTR gbsMember    = NULL; // <Member to check> 
	BSTR gbsUSER      = NULL;
	BSTR gbsPASS      = NULL; // <Password used for binding to the DC>



LDAPAUTH_API BOOL CUGP(char * userin,char *password,char *machine,char *group,int locdom);

