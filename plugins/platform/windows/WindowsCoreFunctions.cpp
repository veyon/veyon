/*
 * WindowsCoreFunctions.cpp - implementation of WindowsCoreFunctions class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#include <QDir>

#include <shlobj.h>

#include "WindowsCoreFunctions.h"
#include "XEventLog.h"


static QString windowsConfigPath( REFKNOWNFOLDERID folderId )
{
	QString result;

	wchar_t* path = nullptr;
	if( SHGetKnownFolderPath( folderId, KF_FLAG_DEFAULT, nullptr, &path ) == S_OK )
	{
		result = QString::fromWCharArray( path );
		CoTaskMemFree( path );
	}

	return result;
}



WindowsCoreFunctions::WindowsCoreFunctions() :
	m_eventLog( nullptr )
{
}



WindowsCoreFunctions::~WindowsCoreFunctions()
{
	delete m_eventLog;
}



QString WindowsCoreFunctions::personalAppDataPath() const
{
	return windowsConfigPath( FOLDERID_RoamingAppData ) + QDir::separator() + QStringLiteral("Veyon") + QDir::separator();
}



QString WindowsCoreFunctions::globalAppDataPath() const
{
	return windowsConfigPath( FOLDERID_ProgramData ) + QDir::separator() + QStringLiteral("Veyon") + QDir::separator();
}



void WindowsCoreFunctions::initNativeLoggingSystem( const QString& appName )
{
	m_eventLog = new CXEventLog( (wchar_t*) appName.utf16() );
}



void WindowsCoreFunctions::writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel )
{
	WORD wtype = -1;

	switch( loglevel )
	{
	case Logger::LogLevelCritical:
	case Logger::LogLevelError: wtype = EVENTLOG_ERROR_TYPE; break;
	case Logger::LogLevelWarning: wtype = EVENTLOG_WARNING_TYPE; break;
		//case LogLevelInfo: wtype = EVENTLOG_INFORMATION_TYPE; break;
	default:
		break;
	}

	if( wtype > 0 )
	{
		m_eventLog->Write( wtype, (wchar_t*) message.utf16() );
	}
}
