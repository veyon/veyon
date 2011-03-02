/*
 * SystemConfigurationModifier.cpp - class for easy modification of iTALC-related
 *                                   settings in the operating system
 *
 * Copyright (c) 2010-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <italcconfig.h>

#ifdef ITALC_BUILD_WIN32
#define INITGUID
#include <windows.h>
#include <netfw.h>
#endif

#include "SystemConfigurationModifier.h"
#include "ImcCore.h"
#include "Logger.h"


bool SystemConfigurationModifier::setServiceAutostart( bool enabled )
{
#ifdef ITALC_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, "icas", SERVICE_ALL_ACCESS );
	if( !hservice )
	{
		ilog_failed( "OpenService()" );
		CloseServiceHandle( hsrvmanager );
		return false;
	}

	if( !ChangeServiceConfig( hservice,
					SERVICE_NO_CHANGE,	// dwServiceType
					enabled ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
					SERVICE_NO_CHANGE,	// dwErrorControl
					NULL,	// lpBinaryPathName
					NULL,	// lpLoadOrderGroup
					NULL,	// lpdwTagId
					NULL,	// lpDependencies
					NULL,	// lpServiceStartName
					NULL,	// lpPassword
					NULL	// lpDisplayName
				) )
	{
		ilog_failed( "ChangeServiceConfig()" );
		CloseServiceHandle( hservice );
		CloseServiceHandle( hsrvmanager );

		return false;
	}

	CloseServiceHandle( hservice );
	CloseServiceHandle( hsrvmanager );
#endif

	return true;
}




bool SystemConfigurationModifier::setServiceArguments( const QString &serviceArgs )
{
	bool err = false;
#ifdef ITALC_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, "icas", SERVICE_ALL_ACCESS );
	if( !hservice )
	{
		ilog_failed( "OpenService()" );
		CloseServiceHandle( hsrvmanager );
		return false;
	}

	QString binaryPath = QString( "\"%1\" -service %2" ).
								arg( ImcCore::icaFilePath() ).
								arg( serviceArgs );

	if( !ChangeServiceConfig( hservice,
					SERVICE_NO_CHANGE,	// dwServiceType
					SERVICE_NO_CHANGE,	// dwStartType
					SERVICE_NO_CHANGE,	// dwErrorControl
					binaryPath.toUtf8().constData(),	// lpBinaryPathName
					NULL,	// lpLoadOrderGroup
					NULL,	// lpdwTagId
					NULL,	// lpDependencies
					NULL,	// lpServiceStartName
					NULL,	// lpPassword
					NULL	// lpDisplayName
				) )
	{
		ilog_failed( "ChangeServiceConfig()" );
		err = true;
	}

	CloseServiceHandle( hservice );
	CloseServiceHandle( hsrvmanager );
#endif

	return !err;
}



#ifdef ITALC_BUILD_WIN32
HRESULT WindowsFirewallInitialize( INetFwProfile **fwProfile )
{
	HRESULT hr = S_OK;
	INetFwMgr* fwMgr = NULL;
	INetFwPolicy* fwPolicy = NULL;

	*fwProfile = NULL;

	// Create an instance of the firewall settings manager.
	hr = CoCreateInstance( CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER,
							IID_INetFwMgr, (void**)&fwMgr );
	if( FAILED( hr ) )
	{
		ilog_failedf( "CoCreateInstance()", "0x%08lx\n", hr );
		goto error;
	}

	// Retrieve the local firewall policy.
	hr = fwMgr->get_LocalPolicy(&fwPolicy);
	if( FAILED( hr ) )
	{
		ilog_failedf( "get_LocalPolicy()", "0x%08lx\n", hr );
		goto error;
	}

	// Retrieve the firewall profile currently in effect.
	hr = fwPolicy->get_CurrentProfile(fwProfile);
	if( FAILED( hr ) )
	{
		ilog_failedf( "get_CurrentProfile()", "0x%08lx\n", hr );
		goto error;
	}

error:
	if( fwPolicy != NULL )
	{
		fwPolicy->Release();
	}

	if( fwMgr != NULL )
	{
		fwMgr->Release();
	}

	return hr;
}


void WindowsFirewallCleanup( INetFwProfile *fwProfile )
{
	if( fwProfile != NULL )
	{
		fwProfile->Release();
	}
}



HRESULT WindowsFirewallAddApp( INetFwProfile* fwProfile,
								const wchar_t* fwProcessImageFileName,
								const wchar_t* fwName )
{
	HRESULT hr = S_OK;
	BSTR fwBstrName = NULL;
	BSTR fwBstrProcessImageFileName = NULL;
	INetFwAuthorizedApplication* fwApp = NULL;
	INetFwAuthorizedApplications* fwApps = NULL;

	// Retrieve the authorized application collection.
	hr = fwProfile->get_AuthorizedApplications( &fwApps );
	if( FAILED( hr ) )
	{
		ilog_failedf( "get_AuthorizedApplications()", "0x%08lx\n", hr );
		goto error;
	}

	// Create an instance of an authorized application.
	hr = CoCreateInstance( CLSID_NetFwAuthorizedApplication, NULL,
							CLSCTX_INPROC_SERVER,
							IID_INetFwAuthorizedApplication, (void**)&fwApp );
	if( FAILED( hr ) )
	{
		ilog_failedf( "CoCreateInstance()", "0x%08lx\n", hr );
		goto error;
	}

	// Allocate a BSTR for the process image file name.
	fwBstrProcessImageFileName = SysAllocString( fwProcessImageFileName );
	if( fwBstrProcessImageFileName == NULL )
	{
		hr = E_OUTOFMEMORY;
		ilog_failedf( "SysAllocString()", "0x%08lx\n", hr );
		goto error;
	}

	// Set the process image file name.
	hr = fwApp->put_ProcessImageFileName( fwBstrProcessImageFileName );
	if( FAILED( hr ) )
	{
		ilog_failedf( "put_ProcessImageFileName()", "0x%08lx\n", hr );
		goto error;
	}

	// Allocate a BSTR for the application friendly name.
	fwBstrName = SysAllocString( fwName );
	if( SysStringLen( fwBstrName ) == 0 )
	{
		hr = E_OUTOFMEMORY;
		ilog_failedf( "SysAllocString()", "0x%08lx\n", hr );
		goto error;
	}

	// Set the application friendly name.
	hr = fwApp->put_Name( fwBstrName );
	if( FAILED( hr ) )
	{
		ilog_failedf( "put_Name()", "0x%08lx\n", hr );
		goto error;
	}

	// Add the application to the collection.
	hr = fwApps->Add( fwApp );
	if( FAILED( hr ) )
	{
		ilog_failedf( "Add()", "0x%08lx\n", hr );
		goto error;
	}

error:
	// Free the BSTRs.
	SysFreeString( fwBstrName );
	SysFreeString( fwBstrProcessImageFileName );

	// Release the authorized application instance.
	if( fwApp != NULL )
	{
		fwApp->Release();
	}

	// Release the authorized application collection.
	if( fwApps != NULL )
	{
		fwApps->Release();
	}

	return hr;
}



HRESULT WindowsFirewallRemoveApp( INetFwProfile* fwProfile,
								const wchar_t* fwProcessImageFileName )
{
	HRESULT hr = S_OK;
	BSTR fwBstrProcessImageFileName = NULL;
	INetFwAuthorizedApplications* fwApps = NULL;

	// Retrieve the authorized application collection.
	hr = fwProfile->get_AuthorizedApplications( &fwApps );
	if( FAILED( hr ) )
	{
		ilog_failedf( "get_AuthorizedApplications()", "0x%08lx\n", hr );
		goto error;
	}

	// Allocate a BSTR for the process image file name.
	fwBstrProcessImageFileName = SysAllocString( fwProcessImageFileName );
	if( fwBstrProcessImageFileName == NULL )
	{
		hr = E_OUTOFMEMORY;
		ilog_failedf( "SysAllocString()", "0x%08lx\n", hr );
		goto error;
	}

	// Remove the application from the collection.
	hr = fwApps->Remove( fwBstrProcessImageFileName );
	if( FAILED( hr ) )
	{
		ilog_failedf( "Remove()", "0x%08lx\n", hr );
		goto error;
	}

error:
	// Free the BSTRs.
	SysFreeString( fwBstrProcessImageFileName );

	// Release the authorized application collection.
	if( fwApps != NULL )
	{
		fwApps->Release();
	}

	return hr;
}



HRESULT WindowsFirewallInitialize2( INetFwPolicy2 **fwPolicy2 )
{
	HRESULT hr = S_OK;

	// Create an instance of the firewall settings manager.
	hr = CoCreateInstance( CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER,
							IID_INetFwPolicy2, (void**)fwPolicy2 );
	if( FAILED( hr ) )
	{
		ilog_failedf( "CoCreateInstance()", "0x%08lx\n", hr );
	}

	return hr;
}


void WindowsFirewallCleanup2( INetFwPolicy2 *fwPolicy2 )
{
	if( fwPolicy2 != NULL )
	{
		fwPolicy2->Release();
	}
}



HRESULT WindowsFirewallAddApp2( INetFwPolicy2* fwPolicy2,
								const wchar_t* fwApplicationPath,
								const wchar_t* fwName )
{
	HRESULT hr = S_OK;
	BSTR fwBstrRuleName = NULL;
	BSTR fwBstrApplicationPath = NULL;
	BSTR fwBstrRuleDescription = NULL;
	BSTR fwBstrRuleGrouping = NULL;

	INetFwRules *pFwRules = NULL;
	INetFwRule *pFwRule = NULL;

	// Retrieve INetFwRules
	hr = fwPolicy2->get_Rules( &pFwRules );
	if( FAILED( hr ) )
	{
		ilog_failedf( "get_Rules()", "0x%08lx\n", hr );
		goto cleanup;
	}

	// Create an instance of an authorized application.
	hr = CoCreateInstance( CLSID_NetFwRule, NULL,
							CLSCTX_INPROC_SERVER,
							IID_INetFwRule, (void**)&pFwRule );
	if( FAILED( hr ) )
	{
		ilog_failedf( "CoCreateInstance()", "0x%08lx\n", hr );
		goto cleanup;
	}

	fwBstrRuleName = SysAllocString( fwName );
	fwBstrApplicationPath = SysAllocString( fwApplicationPath );
	fwBstrRuleDescription = SysAllocString( fwName );
	fwBstrRuleGrouping = SysAllocString( fwName );

	// Set the rule
	pFwRule->put_Name( fwBstrRuleName );
	pFwRule->put_ApplicationName( fwBstrApplicationPath );
	pFwRule->put_Description( fwBstrRuleDescription );
	pFwRule->put_Grouping( fwBstrRuleGrouping );

	pFwRule->put_Action( NET_FW_ACTION_ALLOW );
	pFwRule->put_Enabled( VARIANT_TRUE );
	pFwRule->put_Protocol( NET_FW_IP_PROTOCOL_TCP );
	pFwRule->put_Profiles( NET_FW_PROFILE2_ALL );


	// Add the application to the collection.
	hr = pFwRules->Add( pFwRule );
	if( FAILED( hr ) )
	{
		ilog_failedf( "Add()", "0x%08lx\n", hr );
		goto cleanup;
	}

cleanup:
	// Free the BSTRs.
	SysFreeString( fwBstrRuleName );
	SysFreeString( fwBstrApplicationPath );
	SysFreeString( fwBstrRuleDescription );
	SysFreeString( fwBstrRuleGrouping );

	if( pFwRule != NULL )
	{
		pFwRule->Release();
	}

	if( pFwRules != NULL )
	{
		pFwRules->Release();
	}

	return hr;
}



HRESULT WindowsFirewallRemoveApp2( INetFwPolicy2 * fwPolicy2,
									const wchar_t* fwName )
{
	HRESULT hr = S_OK;
	BSTR fwBstrRuleName = SysAllocString( fwName );

	INetFwRules *pFwRules = NULL;

	// Retrieve INetFwRules
	hr = fwPolicy2->get_Rules( &pFwRules );
	if( FAILED( hr ) )
	{
		ilog_failedf( "get_Rules()", "0x%08lx\n", hr );
		goto cleanup;
	}

	// Remove rule
	hr = pFwRules->Remove( fwBstrRuleName );
	if( FAILED( hr ) )
	{
		ilog_failedf( "Remove()", "0x%08lx\n", hr );
		goto cleanup;
	}

cleanup:
	// Free the BSTRs.
	SysFreeString( fwBstrRuleName );

	if( pFwRules != NULL )
	{
		pFwRules->Release();
	}

	return hr;
}


#endif




bool SystemConfigurationModifier::enableFirewallException( bool enabled )
{
#ifdef ITALC_BUILD_WIN32
	HRESULT hr = S_OK;

	// initialize COM
	HRESULT comInit = CoInitializeEx( 0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );

	// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
	// initialized with a different mode. Since we don't care what the mode is,
	// we'll just use the existing mode.
	if( comInit != RPC_E_CHANGED_MODE )
	{
		hr = comInit;
		if( FAILED( hr ) )
		{
			ilog_failedf( "CoInitializeEx()"," 0x%08lx\n", hr );
			return false;
		}
	}

	static const wchar_t *fwRuleName = L"iTALC Client Application";

	const QString p = ImcCore::icaFilePath().replace( '/', '\\' );

	wchar_t icaPath[MAX_PATH];
	p.toWCharArray( icaPath );
	icaPath[p.size()] = 0;

	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	GetVersionEx( &ovi );

	// Windows >= Vista ?
	if( ovi.dwMajorVersion >= 6 )
	{
		// code for advanced firewall API

		// retrieve current firewall profile
		INetFwPolicy2 *fwPolicy2 = NULL;
		hr = WindowsFirewallInitialize2( &fwPolicy2 );
		if( FAILED( hr ) )
		{
			ilog_failedf( "WindowsFirewallInitialize2()", "0x%08lx\n", hr );
			return false;
		}

		// always remove firewall exception first
		hr = WindowsFirewallRemoveApp2( fwPolicy2, fwRuleName );

		if( enabled )
		{
			// add ICA to the list of authorized applications
			hr = WindowsFirewallAddApp2( fwPolicy2, icaPath, fwRuleName );
			if( FAILED( hr ) )
			{
				ilog_failedf( "WindowsFirewallAddApp2()", "0x%08lx\n", hr );
				return false;
			}
		}

		WindowsFirewallCleanup2( fwPolicy2 );
	}
	else
	{
		// code for old firewall API

		// retrieve current firewall profile
		INetFwProfile *fwProfile = NULL;
		hr = WindowsFirewallInitialize( &fwProfile );
		if( FAILED( hr ) )
		{
			ilog_failedf( "WindowsFirewallInitialize()", "0x%08lx\n", hr );
			return false;
		}

		// always remove firewall exception first
		hr = WindowsFirewallRemoveApp( fwProfile, icaPath );

		if( enabled )
		{
			// add ICA to the list of authorized applications
			hr = WindowsFirewallAddApp( fwProfile, icaPath, fwRuleName );
			if( FAILED( hr ) )
			{
				ilog_failedf( "WindowsFirewallAddApp()", "0x%08lx\n", hr );
				return false;
			}
		}

		WindowsFirewallCleanup( fwProfile );
	}

	// Uninitialize COM.
	if( SUCCEEDED( comInit ) )
	{
		CoUninitialize();
	}
#endif

	return true;
}

