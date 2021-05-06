/*
 * DocumentationFigureCreator.h - helper for creating documentation figures
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QEventLoop>
#include <QObject>

#include "VeyonMaster.h"

#ifdef VEYON_DEBUG

class DocumentationFigureCreator : public QObject
{
	Q_OBJECT
public:
	DocumentationFigureCreator() = default;

	void run();

private:
	static constexpr int DialogDelay = 250;

	void createFeatureFigures();
	void createContextMenuFigure();
	void createLogonDialogFigure();
	void createLocationDialogFigure();
	void createScreenshotManagementPanelFigure();
	void createDemoMenuFigure();
	void createPowerDownOptionsFigure();
	void createPowerDownTimeInputDialogFigure();
	void createUserLoginDialogFigure();
	void createTextMessageDialogFigure();
	void createOpenWebsiteDialogFigure();
	void createWebsiteMenuFigure();
	void createRunProgramDialogFigure();
	void createProgramMenuFigure();
	void createRemoteAccessHostDialogFigure();
	void createRemoteAccessWindowFigure();
	void createFileTransferDialogFigure();

	void hideComputers();

	void scheduleUiOperation( const std::function<void(void)>& operation )
	{
		scheduleUiOperation( operation, this );
	}

	static void scheduleUiOperation( const std::function<void(void)>& operation, QObject* context );

	static void grabMenu( QWidget* window, const QString& buttonName, const QString& fileName );
	static void grabWidget( QWidget* widget, const QPoint& pos, const QSize& size, const QString& fileName );
	static void grabDialog( QDialog* dialog, const QSize& size, const QString& fileName );
	static void grabWindow( QWidget* widget, const QString& fileName );
	static void grabWindow( QWidget* widget, const QPoint& pos, const QSize& size, const QString& fileName );

	VeyonMaster m_master{VeyonCore::instance()};
	QEventLoop m_eventLoop{};

} ;

#endif
