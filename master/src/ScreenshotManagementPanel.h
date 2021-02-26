/*
 * ScreenshotManagementPanel.h - declaration of screenshot management view
 *
 * Copyright (c) 2004-2021 Tobias Junghans <tobydox@veyon.io>
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
#include <QWidget>

class QModelIndex;
class Screenshot;

namespace Ui {
class ScreenshotManagementPanel;
}

class ScreenshotManagementPanel : public QWidget
{
	Q_OBJECT
public:
	explicit ScreenshotManagementPanel( QWidget *parent = nullptr );
	~ScreenshotManagementPanel() override;

	void setPreview( const Screenshot& screenshot );

protected:
	void resizeEvent( QResizeEvent* event ) override;

private:
	void updateModel();

	QString filePath( const QModelIndex& index ) const;

	void updateScreenshot( const QModelIndex& index );
	void screenshotDoubleClicked( const QModelIndex& index );

	void showScreenshot();
	void deleteScreenshot();

	Ui::ScreenshotManagementPanel* ui;

	QStringListModel m_model{this};
	QFileSystemWatcher m_fsWatcher{this};

	QTimer m_reloadTimer{this};

	static constexpr auto FsModelResetDelay = 1000;

} ;
