/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
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

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32

#include "Win32AclEditor.h"
#include "../ica/win32/addon/ms-logon/authSSP/vncAccessControl.h"

SI_ACCESS g_vncAccess[] = {
	// these are a easy-to-swallow listing of basic rights for VNC
	{ &GUID_NULL, 0x00000003, L"Full control", SI_ACCESS_GENERAL  }
};

// Here's my crufted-up mapping for VNC generic rights
GENERIC_MAPPING g_vncGenericMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE,
	STANDARD_RIGHTS_REQUIRED
};


HINSTANCE g_hInst;

STDMETHODIMP ItalcSecurityInfo::QueryInterface( REFIID iid, void** ppv )
{
	if ( IID_IUnknown == iid || IID_ISecurityInformation == iid )
		*ppv = static_cast<ISecurityInformation*>(this);
	else return (*ppv = 0), E_NOINTERFACE;
	reinterpret_cast<IUnknown*>( *ppv )->AddRef();
	return S_OK;
}
STDMETHODIMP_(ULONG) ItalcSecurityInfo::AddRef()
{
	return ++m_cRefs;
}
STDMETHODIMP_(ULONG) ItalcSecurityInfo::Release()
{
	ULONG n = --m_cRefs;
	if ( 0 == n )
		delete this;
	return n;
}

STDMETHODIMP
ItalcSecurityInfo::GetObjectInformation( SI_OBJECT_INFO* poi ){
	// We want to edit the DACL (PERMS).
	poi->dwFlags = SI_EDIT_PERMS | SI_NO_ACL_PROTECT;

	// this determines the module used to discover stringtable entries
	poi->hInstance		= g_hInst;
	poi->pszServerName	= (WCHAR *) L""; // Todo(?): Here we need the DC??
	// then also set dwFlags |= SI_SERVER_IS_DC
	poi->pszObjectName	= const_cast<wchar_t*>( m_pszObjectName );
	poi->pszPageTitle	= const_cast<wchar_t*>( m_pszPageTitle );

	if ( m_pszPageTitle )
		poi->dwFlags |= SI_PAGE_TITLE;

	return S_OK;
}

STDMETHODIMP
ItalcSecurityInfo::GetSecurity(SECURITY_INFORMATION ri,
							 PSECURITY_DESCRIPTOR *ppsd,
							 BOOL bDefault){
	vncAccessControl vncAC;
	return (*ppsd = vncAC.GetSD()) ? S_OK : E_FAIL;
}

STDMETHODIMP
ItalcSecurityInfo::SetSecurity(SECURITY_INFORMATION ri, void* psd){
	vncAccessControl vncAC;
	return vncAC.SetSD((PSECURITY_DESCRIPTOR) psd) ? S_OK : E_FAIL;
}

STDMETHODIMP
ItalcSecurityInfo::PropertySheetPageCallback(HWND hwnd, UINT msg, SI_PAGE_TYPE pt){
	// this is effectively a pass-through from the PropertySheet callback,
	// which we don't care about here.
	return S_OK;
}

STDMETHODIMP
ItalcSecurityInfo::GetAccessRights(const GUID*,
								 DWORD dwFlags,
								 SI_ACCESS** ppAccess,
								 ULONG* pcAccesses,
								 ULONG* piDefaultAccess){
	// here's where we hand back the permissions->strings mapping
	*ppAccess = const_cast<SI_ACCESS*>( g_vncAccess );
	*pcAccesses = sizeof g_vncAccess / sizeof *g_vncAccess;
	*piDefaultAccess = 0;
	return S_OK;
}

STDMETHODIMP
ItalcSecurityInfo::MapGeneric(const GUID*, UCHAR* pAceFlags, ACCESS_MASK* pMask){
	// here's where we hand back the generic permissions mapping
	MapGenericMask(pMask, const_cast<GENERIC_MAPPING*>(&g_vncGenericMapping));
	return S_OK;
}

STDMETHODIMP
ItalcSecurityInfo::GetInheritTypes(SI_INHERIT_TYPE** ppInheritTypes, ULONG* pcInheritTypes){
	// We don't need inheritance here.
	*ppInheritTypes = NULL;
	*pcInheritTypes = 0;
	return S_OK;
}


void Win32AclEditor( HWND hwnd )
{
	g_hInst = GetModuleHandle( NULL );
	// Convert ISecurityInformation implementation into property pages
	ItalcSecurityInfo* psi = new ItalcSecurityInfo(L"iTALC Server", L"iTALC Server");
	psi->AddRef();

	HPROPSHEETPAGE hpsp[1];
	hpsp[0] = CreateSecurityPage(psi);
	psi->Release(); // does "delete this"!

	// Wrap the property page in a modal dialog by calling PropertySheet
	PROPSHEETHEADER psh;
	ZeroMemory(&psh, sizeof psh);
	psh.dwSize		= sizeof psh;
	psh.hwndParent	= hwnd;
	psh.pszCaption	= _T("iTALC Logon Authentication");
	psh.nPages		= sizeof hpsp / sizeof *hpsp;
	psh.phpage		= hpsp;

	PropertySheet(&psh);
}

#endif
