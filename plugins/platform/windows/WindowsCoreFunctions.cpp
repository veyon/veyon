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



QString WindowsCoreFunctions::programFileExtension() const
{
	return QStringLiteral( ".exe" );
}



QString WindowsCoreFunctions::personalAppDataPath() const
{
	return windowsConfigPath( FOLDERID_RoamingAppData ) + QDir::separator() + QStringLiteral("Veyon") + QDir::separator();
}



QString WindowsCoreFunctions::globalAppDataPath() const
{
	return windowsConfigPath( FOLDERID_ProgramData ) + QDir::separator() + QStringLiteral("Veyon") + QDir::separator();
}
