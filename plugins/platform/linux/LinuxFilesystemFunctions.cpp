/*
 * LinuxFilesystemFunctions.cpp - implementation of LinuxFilesystemFunctions class
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>

#include "LinuxFilesystemFunctions.h"


QString LinuxFilesystemFunctions::personalAppDataPath() const
{
	return QDir::homePath() + QDir::separator() + QStringLiteral(".veyon");
}



QString LinuxFilesystemFunctions::globalAppDataPath() const
{
	return QStringLiteral( "/etc/veyon" );
}



QString LinuxFilesystemFunctions::globalTempPath() const
{
	return QStringLiteral( "/tmp" );

}



QString LinuxFilesystemFunctions::fileOwnerGroup( const QString& filePath )
{
	return QFileInfo( filePath ).group();
}



bool LinuxFilesystemFunctions::setFileOwnerGroup( const QString& filePath, const QString& ownerGroup )
{
	struct stat statBuffer;
	if( stat( filePath.toUtf8().constData(), &statBuffer ) != 0 )
	{
		qCritical() << Q_FUNC_INFO << "failed to stat file" << filePath;
		return false;
	}

	const auto grp = getgrnam( ownerGroup.toUtf8().constData() );
	if( grp == nullptr )
	{
		qCritical() << Q_FUNC_INFO << "failed to get gid for" << ownerGroup;
		return false;
	}

	const auto gid = grp->gr_gid;

	if( chown( filePath.toUtf8().constData(), statBuffer.st_uid, gid ) != 0 ) // Flawfinder:ignore
	{
		qCritical() << Q_FUNC_INFO << "failed to change owner group of file" << filePath;
		return false;
	}

	return true;
}



bool LinuxFilesystemFunctions::setFileOwnerGroupPermissions( const QString& filePath, QFile::Permissions permissions )
{
	QFile file( filePath );

	auto currentPermissions = file.permissions();

	for( auto permissionFlag : { QFile::ReadGroup, QFile::WriteGroup, QFile::ExeGroup } )
	{
		if( permissions & permissionFlag )
		{
			currentPermissions |= permissionFlag;
		}
		else
		{
			currentPermissions &= ~permissionFlag;
		}
	}

	return QFile( filePath ).setPermissions( permissions );
}
