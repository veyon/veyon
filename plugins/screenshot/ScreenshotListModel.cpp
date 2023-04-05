/*
 * ScreenshotListModel.cpp - implementation of ScreenshotListModel
 *
 * Copyright (c) 2021-2023 Tobias Junghans <tobydox@veyon.io>
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
#include <QFileInfo>

#include "Filesystem.h"
#include "ScreenshotListModel.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


ScreenshotListModel::ScreenshotListModel( QObject* parent ) :
	QStringListModel( parent )
{
	VeyonCore::filesystem().ensurePathExists( VeyonCore::config().screenshotDirectory() );

	m_fsWatcher.addPath( VeyonCore::filesystem().screenshotDirectoryPath() );
	connect( &m_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &ScreenshotListModel::updateModel );

	m_reloadTimer.setInterval( FsModelResetDelay );
	m_reloadTimer.setSingleShot( true );

	connect( &m_reloadTimer, &QTimer::timeout, this, &ScreenshotListModel::updateModel );
	connect( &VeyonCore::filesystem(), &Filesystem::screenshotDirectoryModified,
			 &m_reloadTimer, QOverload<>::of(&QTimer::start) );

	updateModel();
}



QHash<int, QByteArray> ScreenshotListModel::roleNames() const
{
	//auto roles = QStringListModel::roleNames();
	QHash<int, QByteArray> roles;
	roles[Qt::DisplayRole] = "name";
	roles[Qt::UserRole+1] = "url";
	return roles;
}



QVariant ScreenshotListModel::data(const QModelIndex& index, int role) const
{
	if( role == Qt::UserRole+1 )
	{
		return QString{QStringLiteral("file://") + filePath(index)};
	}

	return QStringListModel::data(index, role);
}



void ScreenshotListModel::updateModel()
{
	setStringList( QDir{ VeyonCore::filesystem().screenshotDirectoryPath() }
					   .entryList( { QStringLiteral("*.png") }, QDir::Filter::Files, QDir::SortFlag::Name ) );
}



QString ScreenshotListModel::filePath( const QModelIndex& index ) const
{
	return VeyonCore::filesystem().screenshotDirectoryPath() + QDir::separator() +
		   data( index, Qt::DisplayRole ).toString();
}



void ScreenshotListModel::deleteScreenshot( const QModelIndexList& indexes )
{
	for( const auto& index : indexes )
	{
		QFile::remove( filePath( index ) );
	}

	updateModel();
}
