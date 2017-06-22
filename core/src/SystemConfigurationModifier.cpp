/*
 * SystemConfigurationModifier.cpp - class for easy modification of Veyon-related
 *                                   settings in the operating system
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <veyonconfig.h>

#ifdef VEYON_BUILD_WIN32
#define INITGUID
#include <winsock2.h>
#include <windows.h>
#include <netfw.h>
#endif

#include "FeatureWorkerManager.h"
#include "SystemConfigurationModifier.h"
#include "ServiceControl.h"
#include "Logger.h"


static auto serviceName = L"VeyonService";

bool SystemConfigurationModifier::setServiceAutostart( bool enabled )
{
#ifdef VEYON_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, serviceName, SERVICE_ALL_ACCESS );
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
#ifdef VEYON_BUILD_WIN32
	SC_HANDLE hsrvmanager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if( !hsrvmanager )
	{
		ilog_failed( "OpenSCManager()" );
		return false;
	}

	SC_HANDLE hservice = OpenService( hsrvmanager, serviceName, SERVICE_ALL_ACCESS );
	if( !hservice )
	{
		ilog_failed( "OpenService()" );
		CloseServiceHandle( hsrvmanager );
		return false;
	}

	QString binaryPath = QString( "\"%1\" -service %2" ).
								arg( ServiceControl::serviceFilePath() ).
								arg( serviceArgs );

	if( !ChangeServiceConfig( hservice,
					SERVICE_NO_CHANGE,	// dwServiceType
					SERVICE_NO_CHANGE,	// dwStartType
					SERVICE_NO_CHANGE,	// dwErrorControl
					(wchar_t *) binaryPath.utf16(),	// lpBinaryPathName
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



#ifdef VEYON_BUILD_WIN32

static HRESULT WindowsFirewallInitialize2( INetFwPolicy2 **fwPolicy2 )
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


static void WindowsFirewallCleanup2( INetFwPolicy2 *fwPolicy2 )
{
	if( fwPolicy2 != NULL )
	{
		fwPolicy2->Release();
	}
}



static HRESULT WindowsFirewallAddApp2( INetFwPolicy2* fwPolicy2,
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



static HRESULT WindowsFirewallRemoveApp2( INetFwPolicy2 * fwPolicy2,
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

static bool configureFirewallException( INetFwPolicy2* fwPolicy2, const wchar_t* fwApplicationPath, const wchar_t* fwName, bool enabled )
{
	// always remove firewall exception first
	HRESULT hr = WindowsFirewallRemoveApp2( fwPolicy2, fwName );

	if( enabled )
	{
		// add service to the list of authorized applications
		hr = WindowsFirewallAddApp2( fwPolicy2, fwApplicationPath, fwName );
		if( FAILED( hr ) )
		{
			// failed because firewall service not running / disabled?
			if( (DWORD) hr == 0x800706D9 )
			{
				// then assume this is intended, log a warning and
				// pretend everything went well
				qWarning() << "Windows Firewall service not running or disabled - can't add or remove firewall exception!";
				return true;
			}

			ilog_failedf( "WindowsFirewallAddApp2()", "0x%08lx\n", hr );
			return false;
		}
	}

	return true;
}

#endif




bool SystemConfigurationModifier::enableFirewallException( bool enabled )
{
	bool result = true;

#ifdef VEYON_BUILD_WIN32
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

	// retrieve current firewall profile
	INetFwPolicy2 *fwPolicy2 = nullptr;
	hr = WindowsFirewallInitialize2( &fwPolicy2 );
	if( FAILED( hr ) )
	{
		ilog_failedf( "WindowsFirewallInitialize2()", "0x%08lx\n", hr );
		return false;
	}

	const auto serviceFilePath = ServiceControl::serviceFilePath();
	const auto workerFilePath = FeatureWorkerManager::workerProcessFilePath();

	result &= configureFirewallException( fwPolicy2, (const wchar_t *) serviceFilePath.utf16(), L"Veyon Service", enabled );
	result &= configureFirewallException( fwPolicy2, (const wchar_t *) workerFilePath.utf16(), L"Veyon Worker", enabled );

	WindowsFirewallCleanup2( fwPolicy2 );

	// Uninitialize COM.
	if( SUCCEEDED( comInit ) )
	{
		CoUninitialize();
	}
#endif

	return result;
}



bool SystemConfigurationModifier::enableSoftwareSAS( bool enabled )
{
#ifdef VEYON_BUILD_WIN32
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if( RegCreateKeyEx( HKEY_LOCAL_MACHINE,
						L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
						0, REG_NONE, REG_OPTION_NON_VOLATILE,
						KEY_READ, NULL, &hkLocal, &dw ) != ERROR_SUCCESS)
	{
		return false;
	}

	if( RegOpenKeyEx( hkLocal,
					  L"System",
					  0, KEY_WRITE | KEY_READ,
					  &hkLocalKey ) != ERROR_SUCCESS )
	{
		RegCloseKey( hkLocal );
		return false;
	}

	LONG pref = enabled ? 1 : 0;
	RegSetValueEx( hkLocalKey, L"SoftwareSASGeneration", 0, REG_DWORD, (LPBYTE) &pref, sizeof(pref) );
	RegCloseKey( hkLocalKey );
	RegCloseKey( hkLocal );
#endif

	return true;
}
