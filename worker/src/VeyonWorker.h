// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"
#include "VeyonWorkerInterface.h"

class FeatureWorkerManagerConnection;

class VeyonWorker : public QObject, VeyonWorkerInterface
{
	Q_OBJECT
public:
	explicit VeyonWorker(QUuid featureUid, QObject* parent = nullptr);
	~VeyonWorker() override;

	bool sendFeatureMessageReply(const FeatureMessage& reply) override;

	VeyonCore& core()
	{
		return m_core;
	}

private:
	VeyonCore m_core;
	FeatureWorkerManagerConnection* m_workerManagerConnection = nullptr;
};
