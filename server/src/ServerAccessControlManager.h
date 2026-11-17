// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>

#include "AccessControlGracePeriod.h"
#include "DesktopAccessDialog.h"
#include "VncServerClient.h"

class AccessControlProvider;
class VariantArrayMessage;

class ServerAccessControlManager : public QObject
{
	Q_OBJECT
public:
	ServerAccessControlManager(FeatureWorkerManager& featureWorkerManager,
							   DesktopAccessDialog& desktopAccessDialog,
							   AccessControlProvider& accessControlProvider,
							   QObject* parent,
							   const AccessControlGracePeriod::Clock& clock = IpcChannel::steadyNow);

	void addClient(VncServerClient* client);
	void removeClient(VncServerClient* client);

	// tests
	void setGraceRecheckInterval(int interval);
	bool isGraceRecheckActive() const;


Q_SIGNALS:
	void finished(VncServerClient* client);

private:
	static constexpr int ClientWaitInterval = 1000;

	enum class CheckMode {
		NewConnection,
		Recheck,
	};

	void addClient(VncServerClient* client, CheckMode mode);
	void recheckAllClients();
	// true if the client was closed
	bool recheckClient(VncServerClient* client);
	void recheckGraceClients();
	void performAccessControl(VncServerClient* client, CheckMode mode);
	VncServerClient::AccessControlState confirmDesktopAccess(VncServerClient* client);
	void finishDesktopAccessConfirmation(VncServerClient* client);

	QStringList connectedUsers() const;

	FeatureWorkerManager& m_featureWorkerManager;
	DesktopAccessDialog& m_desktopAccessDialog;
	AccessControlProvider& m_accessControlProvider;

	VncServerClientList m_clients;

	using HostUserPair = QPair<QString, QString>;
	using DesktopAccessChoiceMap = QMap<HostUserPair, DesktopAccessDialog::Choice>;

	DesktopAccessChoiceMap m_desktopAccessChoices;

	AccessControlGracePeriod m_gracePeriod;
	QTimer m_graceRecheckTimer{this};
};
