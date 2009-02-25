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
#include "vncSecurityEditor.h"
#include "vncSecurityEditorProps.h"
#include "vncAccessControl.h"


typedef PSECURITY_DESCRIPTOR (*vncGetSDFn)(void);
typedef BOOL (*vncSetSDFn)(PSECURITY_DESCRIPTOR pSD);
vncGetSDFn vncGetSDdll = NULL;
vncSetSDFn vncSetSDdll = NULL;

HINSTANCE g_hInst;

STDMETHODIMP vncSecurityInfo::QueryInterface( REFIID iid, void** ppv )
{
	if ( IID_IUnknown == iid || IID_ISecurityInformation == iid )
		*ppv = static_cast<ISecurityInformation*>(this);
	else return (*ppv = 0), E_NOINTERFACE;
	reinterpret_cast<IUnknown*>( *ppv )->AddRef();
	return S_OK;
}
STDMETHODIMP_(ULONG) vncSecurityInfo::AddRef()
{
	return ++m_cRefs;
}
STDMETHODIMP_(ULONG) vncSecurityInfo::Release()
{
	ULONG n = --m_cRefs;
	if ( 0 == n )
		delete this;
	return n;
}

STDMETHODIMP 
vncSecurityInfo::GetObjectInformation( SI_OBJECT_INFO* poi ){
	// We want to edit the DACL (PERMS).
	poi->dwFlags = SI_EDIT_PERMS | SI_NO_ACL_PROTECT;
	
	// this determines the module used to discover stringtable entries
	poi->hInstance		= g_hInst;
	poi->pszServerName	= L""; // Todo(?): Here we need the DC??
	// then also set dwFlags |= SI_SERVER_IS_DC
	poi->pszObjectName	= const_cast<wchar_t*>( m_pszObjectName );
	poi->pszPageTitle	= const_cast<wchar_t*>( m_pszPageTitle );
	
	if ( m_pszPageTitle )
		poi->dwFlags |= SI_PAGE_TITLE;
	
	return S_OK;
}

STDMETHODIMP 
vncSecurityInfo::GetSecurity(SECURITY_INFORMATION ri, 
							 PSECURITY_DESCRIPTOR *ppsd, 
							 BOOL bDefault){
	vncAccessControl vncAC;
	return (*ppsd = vncAC.GetSD()) ? S_OK : E_FAIL;
}

STDMETHODIMP 
vncSecurityInfo::SetSecurity(SECURITY_INFORMATION ri, void* psd){
	vncAccessControl vncAC;
	return vncAC.SetSD((PSECURITY_DESCRIPTOR) psd) ? S_OK : E_FAIL;
}

STDMETHODIMP 
vncSecurityInfo::PropertySheetPageCallback(HWND hwnd, UINT msg, SI_PAGE_TYPE pt){
	// this is effectively a pass-through from the PropertySheet callback,
	// which we don't care about here.
	return S_OK;
}

STDMETHODIMP 
vncSecurityInfo::GetAccessRights(const GUID*,
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
vncSecurityInfo::MapGeneric(const GUID*, UCHAR* pAceFlags, ACCESS_MASK* pMask){
	// here's where we hand back the generic permissions mapping
	MapGenericMask(pMask, const_cast<GENERIC_MAPPING*>(&g_vncGenericMapping));
	return S_OK;
}

STDMETHODIMP 
vncSecurityInfo::GetInheritTypes(SI_INHERIT_TYPE** ppInheritTypes, ULONG* pcInheritTypes){
	// We don't need inheritance here.
	*ppInheritTypes = NULL;
	*pcInheritTypes = 0;
	return S_OK;
}

AUTHSSP_API void vncEditSecurity(HWND hwnd, HINSTANCE hInstance) {
	if (CheckAclUI()) {
		g_hInst = hInstance;
		// Convert ISecurityInformation implementation into property pages
		vncSecurityInfo* psi = 
			new vncSecurityInfo(L"UltraVNC Server", L"UltraVNC Server");
		psi->AddRef();
		
			HPROPSHEETPAGE hpsp[1];
			hpsp[0] = CreateSecurityPage(psi);
			psi->Release(); // does "delete this"!

			// Wrap the property page in a modal dialog by calling PropertySheet
			PROPSHEETHEADER psh;
			ZeroMemory(&psh, sizeof psh);
			psh.dwSize		= sizeof psh;
			psh.hwndParent	= hwnd;
			psh.pszCaption	= _T("UltraVNC Security Editor");
			psh.nPages		= sizeof hpsp / sizeof *hpsp;
			psh.phpage		= hpsp;
			
			PropertySheet(&psh);
			
	} else {
		MessageBox(NULL, _T("aclui.dll (function EditSecurity()) not available\n")
			_T("with this Operatingsystem/Servicepack.\n")
			_T("Use ACL import/export utility instead."),
			//sz_ID_WINVNC_ERROR,
			_T("Error"),
			MB_OK | MB_ICONEXCLAMATION);
	}
}

bool CheckAclUI()
{
	HMODULE hModule = LoadLibrary(_T("aclui.dll"));
	if (hModule)
	{
		FARPROC test=NULL;
		test=GetProcAddress( hModule, "EditSecurity" );
		FreeLibrary(hModule);
		if (test) { 
			return true;
		}
	}
	return false;
}

