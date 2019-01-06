/*
 * ServiceControl.h - header for the ServiceControl class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include <QFuture>

#include "VeyonCore.h"

class QWidget;

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT ServiceControl : public QObject
{
	Q_OBJECT
public:
	ServiceControl( const QString& name,
					const QString& filePath,
					const QString& displayName,
					QWidget* parent );

	bool isServiceRegistered();
	void registerService();
	void unregisterService();

	bool isServiceRunning();

	void startService();
	void stopService();


private:
	typedef QFuture<void> Operation;
	void serviceControl( const QString& title, const Operation& operation );
	void graphicalFeedback( const QString &title, const Operation& operation );
	void textFeedback( const QString &title, const Operation& operation );

	const QString m_name;
	const QString m_filePath;
	const QString m_displayName;

	QWidget* m_parent;

};
