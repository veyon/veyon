// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>

#include "FeatureManager.h"
#include "FeatureWorkerManagerConnection.h"
#include "PlatformCoreFunctions.h"
#include "StandardInputReader.h"
#include "VeyonConfiguration.h"
#include "VeyonWorker.h"


VeyonWorker::VeyonWorker(QUuid featureUid, QObject* parent) :
	QObject(parent),
	m_core(QCoreApplication::instance(),
		   VeyonCore::Component::Worker,
		   QStringLiteral("FeatureWorker-") + featureUid.toString(QUuid::WithoutBraces),
		   VeyonCore::defaultInitialization())
{
	const Feature* workerFeature = nullptr;

	for (const auto& feature : VeyonCore::featureManager().features())
	{
		if (feature.uid() == featureUid)
		{
			workerFeature = &feature;
		}
	}

	if (workerFeature == nullptr)
	{
		qFatal("Could not find specified feature");
	}

	if (m_core.config().disabledFeatures().contains(featureUid.toString()))
	{
		qFatal("Specified feature is disabled by configuration!");
	}

	m_workerManagerConnection = new FeatureWorkerManagerConnection(*this, featureUid);

	auto stdInReader = VeyonCore::platform().coreFunctions().createStandardInputReader(this);
	connect(stdInReader, &StandardInputReader::lineRead, this, [this, stdInReader](const QString& line) {
		const auto authToken = line.toUtf8();
		if (authToken.isEmpty() == false)
		{
			m_workerManagerConnection->setAuthToken(authToken);
		}
		else
		{
			vCritical() << "failed to read auth token from stdin";
		}
		stdInReader->deleteLater();
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



bool VeyonWorker::sendFeatureMessageReply(const FeatureMessage& reply)
{
	return m_workerManagerConnection &&
		m_workerManagerConnection->sendMessage(reply);
}
