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
// http://ultravnc.sourceforge.net/

#ifndef _WIN32_ACL_EDITOR_H
#define _WIN32_ACL_EDITOR_H

#include <windows.h>
#include <aclui.h>
#include <aclapi.h>

struct ItalcSecurityInfo : ISecurityInformation
{
	long  m_cRefs;
	const wchar_t* const m_pszObjectName;
	const wchar_t* const m_pszPageTitle;

	ItalcSecurityInfo(const wchar_t* pszObjectName,
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

#endif
