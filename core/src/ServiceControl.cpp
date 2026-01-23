/*
 * ServiceControl.cpp - class for controlling veyon service
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>

#include "PlatformServiceFunctions.h"
#include "ServiceControl.h"
#include "Toast.h"


ServiceControl::ServiceControl( const QString& name,
								const QString& filePath,
								const QString& displayName,
								QWidget* parent ) :
	QObject( parent ),
	m_name( name ),
	m_filePath( filePath ),
	m_displayName( displayName ),
	m_parent( parent )
{
}



bool ServiceControl::isServiceRegistered()
{
	return VeyonCore::platform().serviceFunctions().isRegistered( m_name );
}



bool ServiceControl::isServiceRunning()
{
	return VeyonCore::platform().serviceFunctions().isRunning( m_name );
}



void ServiceControl::startService()
{
	serviceControl(tr("Starting %1").arg(m_displayName),
				   QtConcurrent::run([=]() { VeyonCore::platform().serviceFunctions().start(m_name); }));
}




void ServiceControl::stopService()
{
	serviceControl(tr("Stopping %1").arg(m_displayName),
				   QtConcurrent::run([=]() { VeyonCore::platform().serviceFunctions().stop(m_name); }));
}



void ServiceControl::restartService()
{
	serviceControl(tr("Restarting %1").arg(m_displayName),
				   QtConcurrent::run([=]() {
		VeyonCore::platform().serviceFunctions().stop(m_name);
		VeyonCore::platform().serviceFunctions().start(m_name);
	}));
}



void ServiceControl::registerService()
{
	serviceControl(tr("Registering %1").arg(m_displayName),
				   QtConcurrent::run([=]() { VeyonCore::platform().serviceFunctions().install(m_name,
																							  m_filePath,
																							  PlatformServiceFunctions::StartMode::Auto,
																							  m_displayName ); }));
}



void ServiceControl::unregisterService()
{
	serviceControl(tr("Unregistering %1").arg(m_displayName),
				   QtConcurrent::run([=]() { VeyonCore::platform().serviceFunctions().uninstall( m_name ); }));

}



void ServiceControl::serviceControl(const QString& title, const Operation& operation)
{
	if (m_parent)
	{
		graphicalFeedback(title, operation);
	}
	else
	{
		textFeedback(title, operation);
	}
}



void ServiceControl::graphicalFeedback(const QString& title, const Operation& operation)
{
	auto toast = new Toast(m_parent);
	toast->setTitle(tr("Service control"));
	toast->setText(title);
	toast->applyPreset(Toast::Preset::Information);
	toast->setDuration(0);
	toast->show();

	while (operation.isRunning())
	{
		QCoreApplication::processEvents();
		QThread::msleep(10);
	}

	toast->hide();
}



void ServiceControl::textFeedback(const QString& title, const Operation& operation)
{
	printf( "%s", qUtf8Printable( title ) );

	while( operation.isFinished() == false )
	{
		QCoreApplication::processEvents();
		QThread::msleep( 200 );
		printf( "." );
	}
}
