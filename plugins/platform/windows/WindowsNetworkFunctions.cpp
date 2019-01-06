/*
 * WindowsNetworkFunctions.cpp - implementation of WindowsNetworkFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#define INITGUID
#include <winsock2.h>
#include <windows.h>
#include <netfw.h>
#include <mstcpip.h>

#include <QProcess>

#include "WindowsCoreFunctions.h"
#include "WindowsNetworkFunctions.h"


static HRESULT WindowsFirewallInitialize2( INetFwPolicy2** fwPolicy2 )
{
	HRESULT hr = S_OK;

	// Create an instance of the firewall settings manager.
	hr = CoCreateInstance( CLSID_NetFwPolicy2, nullptr, CLSCTX_INPROC_SERVER,
							IID_INetFwPolicy2, (void**)fwPolicy2 );
	if( FAILED( hr ) )
	{
		qCritical() << Q_FUNC_INFO << "CoCreateInstance() returned" << hr;
	}

	return hr;
}


static void WindowsFirewallCleanup2( INetFwPolicy2* fwPolicy2 )
{
	if( fwPolicy2 != nullptr )
	{
		fwPolicy2->Release();
	}
}



static HRESULT WindowsFirewallAddApp2( INetFwPolicy2* fwPolicy2,
									   const wchar_t* fwApplicationPath,
									   const wchar_t* fwName )
{
	HRESULT hr = S_OK;
	BSTR fwBstrRuleName = nullptr;
	BSTR fwBstrApplicationPath = nullptr;
	BSTR fwBstrRuleDescription = nullptr;
	BSTR fwBstrRuleGrouping = nullptr;

	INetFwRules *pFwRules = nullptr;
	INetFwRule *pFwRule = nullptr;

	// Retrieve INetFwRules
	hr = fwPolicy2->get_Rules( &pFwRules );
	if( FAILED( hr ) )
	{
		qCritical() << Q_FUNC_INFO << "get_Rules() returned" << hr;
		goto cleanup;
	}

	// Create an instance of an authorized application.
	hr = CoCreateInstance( CLSID_NetFwRule, nullptr,
							CLSCTX_INPROC_SERVER,
							IID_INetFwRule, (void**)&pFwRule );
	if( FAILED( hr ) )
	{
		qCritical() << Q_FUNC_INFO << "CoCreateInstance() returned" << hr;
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
		qCritical() << Q_FUNC_INFO << "Add() returned" << hr;
		goto cleanup;
	}

cleanup:
	// Free the BSTRs.
	SysFreeString( fwBstrRuleName );
	SysFreeString( fwBstrApplicationPath );
	SysFreeString( fwBstrRuleDescription );
	SysFreeString( fwBstrRuleGrouping );

	if( pFwRule != nullptr )
	{
		pFwRule->Release();
	}

	if( pFwRules != nullptr )
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

	INetFwRules *pFwRules = nullptr;

	// Retrieve INetFwRules
	hr = fwPolicy2->get_Rules( &pFwRules );
	if( FAILED( hr ) )
	{
		qCritical() << Q_FUNC_INFO << "get_Rules() returned" << hr;
		goto cleanup;
	}

	// Remove rule
	hr = pFwRules->Remove( fwBstrRuleName );
	if( FAILED( hr ) )
	{
		qCritical() << Q_FUNC_INFO << "Remove() returned" << hr;
		goto cleanup;
	}

cleanup:
	// Free the BSTRs.
	SysFreeString( fwBstrRuleName );

	if( pFwRules != nullptr )
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

			qCritical() << Q_FUNC_INFO << "WindowsFirewallAddApp2() returned" << hr;
			return false;
		}
	}

	return true;
}



bool WindowsNetworkFunctions::ping( const QString& hostAddress )
{
	QProcess pingProcess;
	pingProcess.start( QStringLiteral("ping"), { "-n", "1", "-w", QString::number( PingTimeout ), hostAddress } );
	pingProcess.waitForFinished( PingProcessTimeout );

	return pingProcess.exitCode() == 0;
}



bool WindowsNetworkFunctions::configureFirewallException( const QString& applicationPath, const QString& description, bool enabled )
{
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
			qCritical() << Q_FUNC_INFO << "CoInitializeEx() returned" << hr;
			return false;
		}
	}

	// retrieve current firewall profile
	INetFwPolicy2 *fwPolicy2 = nullptr;
	hr = WindowsFirewallInitialize2( &fwPolicy2 );
	if( FAILED( hr ) )
	{
		qCritical() << Q_FUNC_INFO << "WindowsFirewallInitialize2() returned" << hr;
		return false;
	}

	bool result = ::configureFirewallException( fwPolicy2,
												WindowsCoreFunctions::toConstWCharArray( applicationPath ),
												WindowsCoreFunctions::toConstWCharArray( description ),
												enabled );

	WindowsFirewallCleanup2( fwPolicy2 );

	// Uninitialize COM.
	if( SUCCEEDED( comInit ) )
	{
		CoUninitialize();
	}

	return result;
}



bool WindowsNetworkFunctions::configureSocketKeepalive( int socket, bool enabled, int idleTime, int interval, int probes )
{
	DWORD optval;
	DWORD optlen = sizeof(optval);

	optval = enabled ? 1 : 0;
	if( setsockopt( socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char *>( &optval ), optlen ) != 0 )
	{
		int error = WSAGetLastError();
		qWarning() << Q_FUNC_INFO << "could not set SO_KEEPALIVE" << error;
		return false;
	}

	tcp_keepalive keepalive;
	keepalive.onoff = enabled ? 1 : 0;
	keepalive.keepalivetime = idleTime;
	keepalive.keepaliveinterval = interval;

	DWORD bytesReturned = 0;

	if( WSAIoctl( socket, SIO_KEEPALIVE_VALS, &keepalive, sizeof(keepalive), nullptr, 0,
				  &bytesReturned, nullptr, nullptr ) != 0 )
	{
		int error = WSAGetLastError();
		qWarning() << Q_FUNC_INFO << "could not set keepalive parameters" << error;
		return false;
	}

	return true;
}
