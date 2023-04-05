/*
 * ToolButton.h - declaration of class ToolButton
 *
 * Copyright (c) 2006-2023 Tobias Junghans <tobydox@veyon.io>
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

#include <QToolButton>

#include "VeyonCore.h"

class QToolBar;

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT ToolButton : public QToolButton
{
	Q_OBJECT
public:
	ToolButton( const QIcon& icon,
				const QString& label,
				const QString& altLabel = {},
				const QString& description = {},
				const QKeySequence& shortcut = QKeySequence() );
	~ToolButton() override = default;

	static void setIconOnlyMode( QWidget* mainWindow, bool enabled );

	static bool iconOnlyMode()
	{
		return s_iconOnlyMode;
	}

	static void setToolTipsDisabled( bool disabled )
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
	void enterEvent( QEnterEvent* event ) override;
#else
	void enterEvent( QEvent* event ) override;
#endif
	void leaveEvent( QEvent * _e ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	QSize sizeHint() const override;

private:
	bool checkForLeaveEvent();

	static bool s_toolTipsDisabled;
	static bool s_iconOnlyMode;

	QIcon m_icon;

	QString m_label;
	QString m_altLabel;
	QString m_descr;

} ;
