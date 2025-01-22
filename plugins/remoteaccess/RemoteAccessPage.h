/*
 *  RemoteAccessPage.h - logic for RemoteAccessPage
 *
 *  Copyright (c) 2019-2025 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#pragma once

#include <QQuickItem>

#include "ComputerControlInterface.h"

class VncView;
class VncViewItem;

// clazy:excludeall=ctor-missing-parent-argument

class RemoteAccessPage : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString computerName READ computerName CONSTANT)
	Q_PROPERTY(QQuickItem* view READ view CONSTANT)
public:
	explicit RemoteAccessPage( const ComputerControlInterface::Pointer& computerControlInterface,
							   bool viewOnly,
							   QQuickItem* parent );
	~RemoteAccessPage() override;

	ComputerControlInterface::Pointer computerControlInterface() const
	{
		return m_computerControlInterface;
	}

	QString computerName() const
	{
		return m_computerControlInterface->computer().name();
	}

	QQuickItem* view() const;

	VncView* vncView() const;

private:
	ComputerControlInterface::Pointer m_computerControlInterface;
	VncViewItem* m_view;
	QQuickItem* m_item;

} ;
