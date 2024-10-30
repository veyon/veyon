/*
 * ToolButton.cpp - implementation of Veyon-tool-button
 *
 * Copyright (c) 2006-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QAction>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>

#include "ToolButton.h"

bool ToolButton::s_toolTipsDisabled = false;
bool ToolButton::s_iconOnlyMode = false;


ToolButton::ToolButton( const QIcon& icon,
						const QString& label,
						const QString& altLabel,
						const QString& description,
						const QKeySequence& shortcut ) :
	m_icon( icon ),
	m_label( label ),
	m_altLabel( altLabel ),
	m_descr( description )
{
	setShortcut( shortcut );

	setIcon(icon);
	setText(label);
	setAutoRaise(true);
	setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextUnderIcon);

	if (m_altLabel.length() > 0)
	{
		connect (this, &ToolButton::toggled, this, [this](bool checked) {
			setText(checked ? m_altLabel : m_label);
		});
	}
}



void ToolButton::setIconOnlyMode( QWidget* mainWindow, bool enabled )
{
	s_iconOnlyMode = enabled;
	const auto toolButtons = mainWindow->findChildren<ToolButton *>();
	for( auto toolButton : toolButtons )
	{
		toolButton->setToolButtonStyle(enabled ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextUnderIcon);
	}
}



void ToolButton::addTo( QToolBar* toolBar )
{
	auto action = toolBar->addWidget( this );
	action->setText( m_label );
}



#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ToolButton::enterEvent( QEnterEvent* event )
#else
void ToolButton::enterEvent( QEvent* event )
#endif
{
	if (!s_toolTipsDisabled && !m_descr.isEmpty())
	{
		QToolTip::showText(mapToGlobal(QPoint(width() / 2, height())), m_descr, this);
	}

	QToolButton::enterEvent( event );
}



void ToolButton::leaveEvent( QEvent* event )
{
	if (checkForLeaveEvent())
	{
		QToolButton::leaveEvent( event );
	}
}



void ToolButton::mousePressEvent( QMouseEvent* event )
{
	QToolTip::hideText();
	QToolButton::mousePressEvent( event );
}



QSize ToolButton::sizeHint() const
{
	const auto sh = QToolButton::sizeHint();
	return QSize(std::max<int>(sh.height() * 1.3, sh.width()), sh.height());
}



bool ToolButton::checkForLeaveEvent()
{
	if( QRect( mapToGlobal( QPoint( 0, 0 ) ), size() ).
			contains( QCursor::pos() ) )
	{
		QTimer::singleShot( 20, this, &ToolButton::checkForLeaveEvent );
	}
	else
	{
		QToolTip::hideText();
		return true;
	}

	return false;
}
