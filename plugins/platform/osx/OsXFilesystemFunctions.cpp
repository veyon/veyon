/*
 * OsXFilesystemFunctions.cpp - implementation of OsXFilesystemFunctions class
 *
 * This file is part of Veyon - https://veyon.io
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
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "Logger.h"
#include "OsXFilesystemFunctions.h"

QString OsXFilesystemFunctions::personalAppDataPath() const
{
	const auto path = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
	return path.isEmpty() ? QDir::homePath() + QDir::separator() + QStringLiteral(".veyon") : path;
}



QString OsXFilesystemFunctions::globalAppDataPath() const
{
	const auto path = QStandardPaths::writableLocation( QStandardPaths::GenericConfigLocation );
	if( path.isEmpty() )
	{
		return QStringLiteral("/Library/Application Support/veyon");
	}

	return QDir(path).filePath(QStringLiteral("veyon"));
}



QString OsXFilesystemFunctions::globalTempPath() const
{
	return QDir::tempPath();
}



QString OsXFilesystemFunctions::fileOwnerGroup( const QString& filePath )
{
	return QFileInfo( filePath ).group();
}



bool OsXFilesystemFunctions::setFileOwnerGroup( const QString& filePath, const QString& ownerGroup )
{
	struct stat statBuffer{};
	if( stat( filePath.toUtf8().constData(), &statBuffer ) != 0 )
	{
		vCritical() << "OsXFilesystemFunctions: failed to stat file" << filePath;
		return false;
	}

	const auto grp = getgrnam( ownerGroup.toUtf8().constData() );
	if( grp == nullptr )
	{
		vCritical() << "OsXFilesystemFunctions: failed to resolve group" << ownerGroup;
		return false;
	}

	if( chown( filePath.toUtf8().constData(), statBuffer.st_uid, grp->gr_gid ) != 0 )
	{
		vCritical() << "OsXFilesystemFunctions: failed to change group ownership for" << filePath;
		return false;
	}

	return true;
}



bool OsXFilesystemFunctions::setFileOwnerGroupPermissions( const QString& filePath, QFile::Permissions permissions )
{
	QFile file( filePath );
	auto currentPermissions = file.permissions();

	for( auto permissionFlag : { QFile::ReadGroup, QFile::WriteGroup, QFile::ExeGroup } )
	{
		if( permissions.testFlag( permissionFlag ) )
		{
			currentPermissions |= permissionFlag;
		}
		else
		{
			currentPermissions &= ~permissionFlag;
		}
	}

	return file.setPermissions( currentPermissions );
}



bool OsXFilesystemFunctions::openFileSafely( QFile* file, QFile::OpenMode openMode, QFile::Permissions permissions )
{
	if( file == nullptr )
	{
		return false;
	}

	int flags = O_NOFOLLOW | O_CLOEXEC;
	if( openMode.testFlag( QFile::ReadOnly ) )
	{
		flags |= O_RDONLY;
	}

	if( openMode.testFlag( QFile::WriteOnly ) )
	{
		flags |= O_WRONLY;
		if( permissions )
		{
			flags |= O_CREAT;
		}
	}

	if( openMode.testFlag( QFile::Append ) )
	{
		flags |= O_APPEND;
	}
	else if( openMode.testFlag( QFile::Truncate ) )
	{
		flags |= O_TRUNC;
	}

	const auto fileMode =
		( permissions.testFlag( QFile::ReadOwner ) || permissions.testFlag( QFile::ReadUser ) ? S_IRUSR : 0 ) |
		( permissions.testFlag( QFile::WriteOwner ) || permissions.testFlag( QFile::WriteUser ) ? S_IWUSR : 0 ) |
		( permissions.testFlag( QFile::ExeOwner ) || permissions.testFlag( QFile::ExeUser ) ? S_IXUSR : 0 ) |
		( permissions.testFlag( QFile::ReadGroup ) ? S_IRGRP : 0 ) |
		( permissions.testFlag( QFile::WriteGroup ) ? S_IWGRP : 0 ) |
		( permissions.testFlag( QFile::ExeGroup ) ? S_IXGRP : 0 ) |
		( permissions.testFlag( QFile::ReadOther ) ? S_IROTH : 0 ) |
		( permissions.testFlag( QFile::WriteOther ) ? S_IWOTH : 0 ) |
		( permissions.testFlag( QFile::ExeOther ) ? S_IXOTH : 0 );

	const int fd = ::open( file->fileName().toUtf8().constData(), flags, fileMode );
	if( fd == -1 )
	{
		return false;
	}

	struct stat statBuffer{};
	if( fstat( fd, &statBuffer ) != 0 )
	{
		::close( fd );
		return false;
	}

	if( statBuffer.st_uid != getuid() )
	{
		::close( fd );
		return false;
	}

	if( fileMode != 0 )
	{
		(void) fchmod( fd, fileMode );
	}

	return file->open( fd, openMode, QFileDevice::AutoCloseHandle );
}
