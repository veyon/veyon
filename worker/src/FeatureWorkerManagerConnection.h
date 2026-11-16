// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTcpSocket>
#include <QTimer>

#include "Feature.h"

class FeatureManager;
class FeatureMessage;
class VeyonWorkerInterface;

class FeatureWorkerManagerConnection : public QObject
{
	Q_OBJECT
public:
	FeatureWorkerManagerConnection(VeyonWorkerInterface& worker,
								   Feature::Uid featureUid,
								   QObject* parent = nullptr);

	void setAuthToken(const QByteArray& authToken);

	bool sendMessage(const FeatureMessage& message);

private:
	static constexpr auto ConnectTimeout = 3000;

	void tryConnection();
	void sendInitMessage();
	void receiveMessage();

	VeyonWorkerInterface& m_worker;
	const int m_port;
	QTcpSocket m_socket;
	Feature::Uid m_featureUid;
	QByteArray m_authToken;
	QTimer m_connectTimer{this};
};
