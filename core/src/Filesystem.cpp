/*
 * Filesystem.cpp - filesystem related query and manipulation functions
 *
 * Copyright (c) 2017-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>
#include <QDir>
#include <QHostInfo>
#include <QStandardPaths>

#include "VeyonConfiguration.h"
#include "Filesystem.h"
#include "PlatformFilesystemFunctions.h"
#include "PlatformUserFunctions.h"


QString Filesystem::expandPath( QString path ) const
{
	const auto p = QDir::toNativeSeparators( path.replace( QStringLiteral( "%HOME%" ), QDir::homePath() ).
											 replace( QStringLiteral( "%HOSTNAME%" ), QHostInfo::localHostName() ).
											 replace( QStringLiteral( "%PROFILE%" ), QDir::homePath() ).
											 replace(QStringLiteral( "%USER%" ), VeyonCore::platform().userFunctions().currentUser()).
											 replace(QStringLiteral("%DESKTOP%"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).
											 replace(QStringLiteral("%DOCUMENTS%"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).
											 replace(QStringLiteral("%DOWNLOADS%"), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).
											 replace(QStringLiteral("%PICTURES%"), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).
											 replace(QStringLiteral("%VIDEOS%"), QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).
											 replace( QStringLiteral( "%APPDATA%" ), VeyonCore::platform().filesystemFunctions().personalAppDataPath() ).
											 replace( QStringLiteral( "%GLOBALAPPDATA%" ), VeyonCore::platform().filesystemFunctions().globalAppDataPath() ).
											 replace(QStringLiteral("%TEMP%"), QDir::tempPath()));

	// remove duplicate directory separators - however skip the first two chars
	// as they might specify an UNC path on Windows
	if( p.length() > 3 )
	{
		return p.left( 2 ) + p.mid( 2 ).replace(
					QString( QStringLiteral( "%1%1" ) ).arg( QDir::separator() ),
					QDir::separator() );
	}

	return p;
}



QString Filesystem::shrinkPath( QString path ) const
{
	path = QDir::toNativeSeparators( path );

	const auto tempPath = QDir::toNativeSeparators(QDir::tempPath());
	const auto personalAppDataPath = VeyonCore::platform().filesystemFunctions().personalAppDataPath();
	const auto globalAppDataPath = VeyonCore::platform().filesystemFunctions().globalAppDataPath();
	const auto desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
	const auto documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	const auto downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
	const auto picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	const auto videosPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
	const auto homePath = QDir::toNativeSeparators(QDir::homePath());

	for (const auto& mapping : {
			 qMakePair(tempPath, QStringLiteral("TEMP")),
			 qMakePair(personalAppDataPath, QStringLiteral("APPDATA")),
			 qMakePair(globalAppDataPath, QStringLiteral("GLOBALAPPDATA")),
			 qMakePair(desktopPath, QStringLiteral("DESKTOP")),
			 qMakePair(documentsPath, QStringLiteral("DOCUMENTS")),
			 qMakePair(downloadsPath, QStringLiteral("DOWNLOADS")),
			 qMakePair(picturesPath, QStringLiteral("PICTURES")),
			 qMakePair(videosPath, QStringLiteral("VIDEOS")),
			 qMakePair(homePath, QStringLiteral("HOME")),
			 })
	{
		if (path.startsWith(mapping.first))
		{
			path.replace(mapping.first, QStringLiteral("%%1%").arg(mapping.second));
			break;
		}
	}

	// remove duplicate directory separators - however skip the first two chars
	// as they might specify an UNC path on Windows
	if( path.length() > 3 )
	{
		return QDir::toNativeSeparators( path.left( 2 ) + path.mid( 2 ).replace(
											 QString( QStringLiteral( "%1%1" ) ).arg( QDir::separator() ), QDir::separator() ) );
	}

	return QDir::toNativeSeparators( path );
}




bool Filesystem::ensurePathExists( const QString &path ) const
{
	const QString expandedPath = VeyonCore::filesystem().expandPath( path );

	if( path.isEmpty() || QDir( expandedPath ).exists() )
	{
		return true;
	}

	vDebug() << "creating " << path << "=>" << expandedPath;

	QString p = expandedPath;

	QStringList dirs;
	while( !QDir( p ).exists() && !p.isEmpty() )
	{
		dirs.push_front( QDir( p ).dirName() );
		p.chop( dirs.front().size() + 1 );
	}

	if( !p.isEmpty() )
	{
		return QDir( p ).mkpath( dirs.join( QDir::separator() ) );
	}

	return false;
}



QString Filesystem::screenshotDirectoryPath() const
{
	return expandPath( VeyonCore::config().screenshotDirectory() );
}



QString Filesystem::serviceFilePath() const
{
	return QDir::toNativeSeparators( QCoreApplication::applicationDirPath() + QDir::separator() +
									 QStringLiteral("veyon-service" ) + VeyonCore::executableSuffix() );
}



QString Filesystem::serverFilePath() const
{
	return QDir::toNativeSeparators( QCoreApplication::applicationDirPath() + QDir::separator() +
									 QStringLiteral("veyon-server" ) + VeyonCore::executableSuffix() );
}



QString Filesystem::workerFilePath() const
{
	return QDir::toNativeSeparators( QCoreApplication::applicationDirPath() + QDir::separator() +
									 QStringLiteral("veyon-worker" ) + VeyonCore::executableSuffix() );
}
