// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	void createStartAppDialogFigure();
	void createAppMenuFigure();
	void createRemoteAccessHostDialogFigure();
	void createRemoteAccessWindowFigure();
	void createFileTransferDialogFigure();

	void hideComputers();

	void scheduleUiOperation(const std::function<void(void)>& operation)
	{
		scheduleUiOperation(operation, this);
	}

	static void scheduleUiOperation(const std::function<void(void)>& operation, QObject* context);

	static void grabMenu(QWidget* window, const QString& buttonName, const QString& fileName);
	static void grabWidget(QWidget* widget, QPoint pos, QSize size, const QString& fileName);
	static void grabDialog(QDialog* dialog, QSize size, const QString& fileName);
	static void grabWindow(QWidget* widget, const QString& fileName);
	static void grabWindow(QWidget* widget, QPoint pos, QSize size, const QString& fileName);

	VeyonMaster m_master{VeyonCore::instance()};
	QEventLoop m_eventLoop{};
};

#endif
