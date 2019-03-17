/*
 * ToolButton.h - declaration of class ToolButton
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
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
				const QString& altLabel = QString(),
				const QString& description = QString(),
				const QKeySequence& shortcut = QKeySequence() );
	~ToolButton() = default;

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

	void addTo( QToolBar * );


protected:
	void enterEvent( QEvent * _e ) override;
	void leaveEvent( QEvent * _e ) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void paintEvent( QPaintEvent * _pe ) override;


signals:
	void mouseLeftButton();


private:
	bool checkForLeaveEvent();

	void updateSize();

	static bool s_toolTipsDisabled;
	static bool s_iconOnlyMode;

	int iconSize() const
	{
		return static_cast<int>( 32 * m_pixelRatio );
	}

	int margin() const
	{
		return static_cast<int>( 8 * m_pixelRatio );
	}

	int roundness() const
	{
		return static_cast<int>( 3 * m_pixelRatio );
	}

	int stepSize() const
	{
		return static_cast<int>( 8 * m_pixelRatio );
	}

	qreal m_pixelRatio;
	QIcon m_icon;
	QPixmap m_pixmap;
	bool m_mouseOver;

	QString m_label;
	QString m_altLabel;
	QString m_descr;

} ;



class ToolButtonTip : public QWidget
{
	Q_OBJECT
public:
	ToolButtonTip( const QIcon& icon, const QString& title, const QString& description,
				QWidget* parent, QWidget* toolButton = nullptr );

	QSize sizeHint( void ) const override;


protected:
	void paintEvent( QPaintEvent * _pe ) override;
	void resizeEvent( QResizeEvent * _re ) override;


private:
	void updateMask( void );

	int margin() const
	{
		return static_cast<int>( 8 * m_pixelRatio );
	}

	const int ROUNDED = 2000;

	qreal m_pixelRatio;
	QPixmap m_pixmap;
	QString m_title;
	QString m_description;

	QImage m_bg;

	QWidget* m_toolButton;

} ;
