/*
 * DemoClient.h - client for demo server
 *
 * Copyright (c) 2006-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QObject>

class VncViewWidget;

class DemoClient : public QObject
{
	Q_OBJECT
public:
	DemoClient( const QString& host, int port, bool fullscreen, const QRect& viewport, QObject* parent = nullptr );
	~DemoClient() override;

private:
	void viewDestroyed( QObject* obj );
	void resizeToplevelWidget();

	QWidget* m_toplevel;
	VncViewWidget* m_vncView;

} ;
