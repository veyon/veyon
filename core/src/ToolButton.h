// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QToolButton>

#include "VeyonCore.h"

class QToolBar;

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT ToolButton : public QToolButton
{
	Q_OBJECT
public:
	ToolButton(const QIcon& icon,
			   const QString& label,
			   const QString& altLabel = {},
			   const QString& description = {},
			   const QKeySequence& shortcut = QKeySequence());
	~ToolButton() override = default;

	static void setIconOnlyMode(QWidget* mainWindow, bool enabled);

	static bool iconOnlyMode()
	{
		return s_iconOnlyMode;
	}

	static void setToolTipsDisabled(bool disabled)
	{
		s_toolTipsDisabled = disabled;
	}

	static bool toolTipsDisabled()
	{
		return s_toolTipsDisabled;
	}

	void addTo(QToolBar* toolBar);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	void enterEvent(QEnterEvent* event) override;
#else
	void enterEvent(QEvent* event) override;
#endif
	void leaveEvent(QEvent* _e) override;
	void mousePressEvent(QMouseEvent* _me) override;
	QSize sizeHint() const override;

private:
	bool checkForLeaveEvent();

	static bool s_toolTipsDisabled;
	static bool s_iconOnlyMode;

	QIcon m_icon;

	QString m_label;
	QString m_altLabel;
	QString m_descr;
};
