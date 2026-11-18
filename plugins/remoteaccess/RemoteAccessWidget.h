// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ComputerControlInterface.h"

#include <QWidget>


class QActionGroup;

class VncView;
class VncViewWidget;
class RemoteAccessWidget;
class QToolButton;
class QLabel;
class QScrollArea;

// clazy:excludeall=ctor-missing-parent-argument

class RemoteAccessWidgetToolBar : public QWidget
{
	Q_OBJECT
public:
	RemoteAccessWidgetToolBar(RemoteAccessWidget* parent,
							  bool startViewOnly, bool showViewOnlyToggleButton);
	~RemoteAccessWidgetToolBar() override = default;

	void updateControls(bool viewOnly);
	void updateScreenSelectActions(int newScreen);
	void updateFullScreen(bool fullScreen);

private:
	void updateConnectionState();
	void updateScreens();

	RemoteAccessWidget* m_parent;
	QToolButton* m_viewButton;
	QToolButton* m_controlButton;
	QToolButton* m_selectScreenButton;
	QToolButton* m_sendShortcutButton;
	QToolButton* m_screenshotButton;
	QToolButton* m_fullScreenButton;
	bool m_viewOnly;

	QActionGroup* m_screenSelectActions{nullptr};

};

class RemoteAccessWidget : public QWidget
{
	Q_OBJECT
public:
	explicit RemoteAccessWidget(const ComputerControlInterface::Pointer& computerControlInterface,
								bool startViewOnly, bool showViewOnlyToggleButton);
	~RemoteAccessWidget() override;

	ComputerControlInterface::Pointer computerControlInterface() const
	{
		return m_computerControlInterface;
	}

	VncView* vncView() const;

	void toggleFullScreen(bool);
	void setViewOnly(bool viewOnly);
	void setFitToWindow(bool fit);
	void takeScreenshot();

protected:
	bool eventFilter(QObject* object, QEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void changeEvent(QEvent* event) override;

private:
	void updateSize();
	void updateRemoteAccessTitle();
	void updateStageGeometry();
	void updateConnectionState();

	ComputerControlInterface::Pointer m_computerControlInterface;
	VncViewWidget* m_vncView;
	RemoteAccessWidgetToolBar* m_toolBar;

	QScrollArea* m_stage;
	QWidget* m_controlBanner;
	QLabel* m_sessionLabel;
	QLabel* m_connectionLabel;
	QLabel* m_modeLabel;
	bool m_fitToWindow{true};
	bool m_viewOnly{true};

	int m_currentScreen{-1};

Q_SIGNALS:
	void screenChangedInRemoteAccessWidget(int newScreen);

};
