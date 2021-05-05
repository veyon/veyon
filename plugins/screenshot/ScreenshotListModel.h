/*
 * ScreenshotListModel.h - declaration of ScreenshotListModel
 *
 * Copyright (c) 2021 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QFileSystemWatcher>
#include <QStringListModel>
#include <QTimer>

class ScreenshotListModel : public QStringListModel
{
	Q_OBJECT
public:
	explicit ScreenshotListModel( QObject *parent = nullptr );

	QHash<int, QByteArray> roleNames() const;

	QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;

	void deleteScreenshot( const QModelIndexList& indexes );

private:
	void updateModel();

	QString filePath( const QModelIndex& index ) const;

	QFileSystemWatcher m_fsWatcher{this};

	QTimer m_reloadTimer{this};

	static constexpr auto FsModelResetDelay = 1000;

} ;
