/*
 * VeyonWorker.cpp - basic implementation of Veyon Worker
 *
 * Copyright (c) 2018-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <iostream>

#include <QCoreApplication>

#include "FeatureManager.h"
#include "FeatureWorkerManagerConnection.h"
#include "PlatformCoreFunctions.h"
#include "VeyonConfiguration.h"
#include "VeyonWorker.h"


VeyonWorker::VeyonWorker(QUuid featureUid, QObject* parent) :
	QObject(parent),
	m_core(QCoreApplication::instance(),
		   VeyonCore::Component::Worker,
		   QStringLiteral("FeatureWorker-") + featureUid.toString(QUuid::WithoutBraces))
{
	const Feature* workerFeature = nullptr;

	for( const auto& feature : VeyonCore::featureManager().features() )
	{
		if( feature.uid() == featureUid )
		{
			workerFeature = &feature;
		}
	}

	if( workerFeature == nullptr )
	{
		qFatal( "Could not find specified feature" );
	}

	if( VeyonCore::config().disabledFeatures().contains( featureUid.toString() ) )
	{
		qFatal( "Specified feature is disabled by configuration!" );
	}

	m_workerManagerConnection = new FeatureWorkerManagerConnection(*this, featureUid);

	VeyonCore::platform().coreFunctions().notifyOnStandardInputReadyRead([this](QObject* notifier) {
		const auto authToken = QTextStream(stdin).readLine().toUtf8();
		if (authToken.isEmpty() == false)
		{
			m_workerManagerConnection->setAuthToken(authToken);
			notifier->deleteLater();
		}
		else
		{
			vCritical() << "failed to read auth token from stdin";
		}
	});

	vInfo() << "Running worker for feature" << workerFeature->name();
}



VeyonWorker::~VeyonWorker()
{
	vDebug();

	delete m_workerManagerConnection;
	m_workerManagerConnection = nullptr;

	vDebug() << "finished";
}



bool VeyonWorker::sendFeatureMessageReply( const FeatureMessage& reply )
{
	return m_workerManagerConnection &&
			m_workerManagerConnection->sendMessage( reply );
}
