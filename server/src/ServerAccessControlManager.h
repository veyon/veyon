// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DesktopAccessDialog.h"
#include "VncServerClient.h"

class VariantArrayMessage;

class ServerAccessControlManager : public QObject
{
	Q_OBJECT
public:
	ServerAccessControlManager(FeatureWorkerManager& featureWorkerManager,
							   DesktopAccessDialog& desktopAccessDialog,
							   QObject* parent);

	void addClient(VncServerClient* client);
	void removeClient(VncServerClient* client);


Q_SIGNALS:
	void finished(VncServerClient* client);

private:
	static constexpr int ClientWaitInterval = 1000;

	void performAccessControl(VncServerClient* client);
	VncServerClient::AccessControlState confirmDesktopAccess(VncServerClient* client);
	void finishDesktopAccessConfirmation(VncServerClient* client);

	QStringList connectedUsers() const;

	FeatureWorkerManager& m_featureWorkerManager;
	DesktopAccessDialog& m_desktopAccessDialog;

	VncServerClientList m_clients;

	using HostUserPair = QPair<QString, QString>;
	using DesktopAccessChoiceMap = QMap<HostUserPair, DesktopAccessDialog::Choice>;

	DesktopAccessChoiceMap m_desktopAccessChoices;
};
