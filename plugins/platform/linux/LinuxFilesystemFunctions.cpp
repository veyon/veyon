/*
 * LinuxFilesystemFunctions.cpp - implementation of LinuxFilesystemFunctions class
 *
 * Copyright (c) 2018-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <fcntl.h>
#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

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
	struct stat statBuffer{};
	if( stat( filePath.toUtf8().constData(), &statBuffer ) != 0 )
	{
		vCritical() << "failed to stat file" << filePath;
		return false;
	}

	const auto grp = getgrnam( ownerGroup.toUtf8().constData() );
	if( grp == nullptr )
	{
		vCritical() << "failed to get gid for" << ownerGroup;
		return false;
	}

	const auto gid = grp->gr_gid;

	if( chown( filePath.toUtf8().constData(), statBuffer.st_uid, gid ) != 0 ) // Flawfinder:ignore
	{
		vCritical() << "failed to change owner group of file" << filePath;
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



bool LinuxFilesystemFunctions::openFileSafely( QFile* file, QIODevice::OpenMode openMode, QFile::Permissions permissions )
{
	if( file == nullptr )
	{
		return false;
	}

	int flags = O_NOFOLLOW | O_CLOEXEC;
	if( openMode & QFile::ReadOnly )
	{
		flags |= O_RDONLY;
	}

	if( openMode & QFile::WriteOnly )
	{
		flags |= O_WRONLY;

		if( permissions )
		{
			flags |= O_CREAT;
		}
	}

	if( openMode & QFile::Append )
	{
		flags |= O_APPEND;
	}
	else if( openMode & QFile::Truncate )
	{
		flags |= O_TRUNC;
	}

	int fileMode =
		( permissions.testFlag( QFile::ReadOwner ) || permissions.testFlag( QFile::ReadUser ) ? S_IRUSR : 0 ) |
		( permissions.testFlag( QFile::WriteOwner ) || permissions.testFlag( QFile::WriteUser ) ? S_IWUSR : 0 ) |
		( permissions.testFlag( QFile::ExeOwner ) || permissions.testFlag( QFile::ExeUser ) ? S_IXUSR : 0 ) |
		( permissions.testFlag( QFile::ReadGroup ) ? S_IRGRP : 0 ) |
		( permissions.testFlag( QFile::WriteGroup ) ? S_IWGRP : 0 ) |
		( permissions.testFlag( QFile::ExeGroup ) ? S_IXGRP : 0 ) |
		( permissions.testFlag( QFile::ReadOther ) ? S_IROTH : 0 ) |
		( permissions.testFlag( QFile::WriteOther ) ? S_IWOTH : 0 ) |
		( permissions.testFlag( QFile::ExeOther ) ? S_IXOTH : 0 );

	int fd = open( file->fileName().toUtf8().constData(), flags, fileMode );
	if( fd == -1 )
	{
		return false;
	}

	struct stat s{};
	if( fstat(fd, &s) != 0 )
	{
		close(fd);
		return false;
	}

	if( s.st_uid != getuid() )
	{
		close(fd);
		return false;
	}

	if( fileMode )
	{
		(void) fchmod( fd, fileMode );
	}

	return file->open( fd, openMode, QFileDevice::AutoCloseHandle );
}



PlatformCoreFunctions::ProcessId LinuxFilesystemFunctions::findFileLockingProcess(const QString& filePath) const
{
	QStringList pidsWithOpenFile;
	QDir procDir(QStringLiteral("/proc"));
	const auto targetCanonicalFilePath = QFileInfo(filePath).canonicalFilePath();

	for (const auto& entry : procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
	{
		bool ok;
		qint64 pid = entry.toLong(&ok);
		if (!ok) continue;

		const auto fdDirPath = QStringLiteral("/proc/%1/fd").arg(entry);
		QDir fdDir(fdDirPath);

		if (!fdDir.exists() || !fdDir.isReadable())
		{
			continue;
		}

		for (const auto& fdEntryInfo : fdDir.entryInfoList(QDir::Files))
		{
			if (fdEntryInfo.isSymLink() &&
				fdEntryInfo.canonicalFilePath() == targetCanonicalFilePath)
			{
				return pid;
			}
		}
	}

	return PlatformCoreFunctions::InvalidProcessId;
}
