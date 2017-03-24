/*
 * ServiceControl.h - header for the ServiceControl class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef SERVICE_CONTROL_H
#define SERVICE_CONTROL_H

#include <QObject>

#include "ItalcCore.h"

class QProcess;
class QWidget;

class ITALC_CORE_EXPORT ServiceControl : public QObject
{
	Q_OBJECT
public:
	ServiceControl( QWidget* parent );

	static QString serviceFilePath();

	bool isServiceRegistered();
	void registerService();
	void unregisterService();

	bool isServiceRunning();

	void startService();
	void stopService();


private:
	void serviceControl( const QString &title, const QString &arg );
	void graphicalFeedback( const QString &title, const QProcess& serviceProcess );
	void textFeedback( const QString &title, const QProcess& serviceProcess );

	QWidget* m_parent;

};

#endif // SERVICE_CONTROL_H
