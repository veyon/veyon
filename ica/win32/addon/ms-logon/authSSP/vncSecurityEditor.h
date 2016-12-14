/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf, B. Braun Melsungen AG. All Rights Reserved.
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
// http://www.uvnc.com

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTHSSP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTHSSP_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef AUTHSSP_EXPORTS
#define AUTHSSP_API __declspec(dllexport)
#else
#define AUTHSSP_API __declspec(dllimport)
#endif

#include <windows.h>
#include <aclui.h>
#include <aclapi.h>
#include <stdio.h>

struct vncSecurityInfo : ISecurityInformation
{
	long  m_cRefs;
	const wchar_t* const m_pszObjectName;
	const wchar_t* const m_pszPageTitle;
	
	vncSecurityInfo(const wchar_t* pszObjectName,
		const wchar_t* pszPageTitle = 0 )
		: m_cRefs(0),
		m_pszObjectName(pszObjectName),
		m_pszPageTitle(pszPageTitle) {}

	STDMETHODIMP QueryInterface( REFIID iid, void** ppv );
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP GetObjectInformation( SI_OBJECT_INFO* poi );
	STDMETHODIMP GetSecurity(SECURITY_INFORMATION ri, PSECURITY_DESCRIPTOR * ppsd, BOOL bDefault);
	STDMETHODIMP SetSecurity(SECURITY_INFORMATION ri, void* psd);
	STDMETHODIMP PropertySheetPageCallback(HWND hwnd, UINT msg, SI_PAGE_TYPE pt);
	STDMETHODIMP GetAccessRights(const GUID*,
								 DWORD dwFlags,
								 SI_ACCESS** ppAccess,
								 ULONG* pcAccesses,
								 ULONG* piDefaultAccess);
	STDMETHODIMP MapGeneric(const GUID*, UCHAR* pAceFlags, ACCESS_MASK* pMask);
	STDMETHODIMP GetInheritTypes(SI_INHERIT_TYPE** ppInheritTypes, ULONG* pcInheritTypes);
};

AUTHSSP_API void vncEditSecurity(HWND hwnd, HINSTANCE hInstance);

bool CheckAclUI();
extern TCHAR *AddToModuleDir(TCHAR *filename, int length);