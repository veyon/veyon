// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AccessControlProvider.h"
#include "DesktopAccessDialog.h"
#include "ServerAccessControlManager.h"
#include "VeyonCore.h"


ServerAccessControlManager::ServerAccessControlManager(FeatureWorkerManager& featureWorkerManager,
													   DesktopAccessDialog& desktopAccessDialog,
													   AccessControlProvider& accessControlProvider,
													   QObject* parent,
													   const AccessControlGracePeriod::Clock& clock) :
	QObject(parent),
	m_featureWorkerManager(featureWorkerManager),
	m_desktopAccessDialog(desktopAccessDialog),
	m_accessControlProvider(accessControlProvider),
	m_clients(),
	m_desktopAccessChoices(),
	m_gracePeriod(clock)
{
	m_graceRecheckTimer.setInterval(AccessControlGracePeriod::RecheckInterval);
	connect(&m_graceRecheckTimer, &QTimer::timeout, this, &ServerAccessControlManager::recheckGraceClients);
}



void ServerAccessControlManager::addClient(VncServerClient* client)
{
	addClient(client, CheckMode::NewConnection);
}



void ServerAccessControlManager::addClient(VncServerClient* client, CheckMode mode)
{
	switch (client->authType())
	{
	case RfbVeyonAuth::KeyFile:
	case RfbVeyonAuth::Logon:
		performAccessControl(client, mode);
		break;

	case RfbVeyonAuth::Token:
		client->setAccessControlState(VncServerClient::AccessControlState::Successful);
		break;

	default:
		// reject unknown auth type
		client->setAccessControlState(VncServerClient::AccessControlState::Failed);
		client->setAccessControlDetails(tr("Requested authentication method not available"));
		break;
	}

	if (client->accessControlState() == VncServerClient::AccessControlState::Successful)
	{
		m_clients.append(client);
	}
}



void ServerAccessControlManager::removeClient(VncServerClient* client)
{
	m_gracePeriod.remove(client);

	// only the departure of a client that had passed access control can change the conditions
	// (e.g. AccessControlRule::Condition::AccessFromAlreadyConnectedUser), not the end of a
	// connection that never got that far
	if (m_clients.removeAll(client) == 0)
	{
		return;
	}

	// force all remaining clients to pass access control again
	recheckAllClients();
}



void ServerAccessControlManager::setGraceRecheckInterval(int interval)
{
	m_graceRecheckTimer.setInterval(interval);
}



bool ServerAccessControlManager::isGraceRecheckActive() const
{
	return m_graceRecheckTimer.isActive();
}



void ServerAccessControlManager::recheckAllClients()
{
	// a client closed by a re-check may have been the reason for the access of others (e.g.
	// AccessControlRule::Condition::AccessFromAlreadyConnectedUser): check the remaining ones again
	// until no further client is closed (terminates, as every further pass closes a client)
	auto clientClosed = true;
	while (clientClosed && m_clients.isEmpty() == false)
	{
		clientClosed = false;

		const VncServerClientList previousClients = m_clients;
		m_clients.clear();

		m_accessControlProvider.beginCheckPass();
		for (auto prevClient : std::as_const(previousClients))
		{
			clientClosed |= recheckClient(prevClient);
		}
		m_accessControlProvider.endCheckPass();
	}
}



bool ServerAccessControlManager::recheckClient(VncServerClient* client)
{
	client->setAccessControlState(VncServerClient::AccessControlState::Init);
	addClient(client, CheckMode::Recheck);

	if (client->accessControlState() != VncServerClient::AccessControlState::Successful &&
		client->accessControlState() != VncServerClient::AccessControlState::Pending)
	{
		vDebug() << "closing connection as client does not pass access control any longer";
		client->setProtocolState(VncServerProtocol::State::Close);
		return true;
	}

	return false;
}



// runs in the server's main thread like all access control checks: a check blocks it until the
// directory broker answers (bounded by the broker's request timeout and, while the directory is
// unavailable, the 3 s circuit breaker of AccessControlDirectoryQueries)
void ServerAccessControlManager::recheckGraceClients()
{
	auto clientClosed = false;

	m_accessControlProvider.beginCheckPass();

	const auto clients = m_gracePeriod.clients();
	for (const auto clientId : clients)
	{
		for (auto client : std::as_const(m_clients))
		{
			if (client == clientId)
			{
				m_clients.removeAll(client);
				clientClosed |= recheckClient(client);
				break;
			}
		}
	}

	m_accessControlProvider.endCheckPass();

	if (clientClosed)
	{
		recheckAllClients();
	}

	if (m_gracePeriod.isEmpty())
	{
		m_graceRecheckTimer.stop();
	}
}



void ServerAccessControlManager::performAccessControl(VncServerClient* client, CheckMode mode)
{
	// implement access control wait for connections other than the one an
	// access dialog is currently active for
	switch (client->accessControlState())
	{
	case VncServerClient::AccessControlState::Init:
		client->accessControlTimer().restart();
		break;
	case VncServerClient::AccessControlState::Waiting:
		if (client->accessControlTimer().elapsed() < ClientWaitInterval)
		{
			return;
		}
		client->accessControlTimer().restart();
		break;
	default:
		break;
	}

	const auto checkResult = m_accessControlProvider.checkAccess(client->username(),
																 client->hostAddress(),
																 connectedUsers());

	const auto description = QStringLiteral("%1@%2").arg(client->username(), client->hostAddress());

	// an established, already authorised connection keeps its access for a while if the directory
	// is not available; new connections are denied (fail closed)
	if (mode == CheckMode::Recheck && checkResult.reason == AccessControlProvider::Reason::DirectoryUnavailable)
	{
		if (m_gracePeriod.keepAccess(client, description))
		{
			client->setAccessControlState(VncServerClient::AccessControlState::Successful);
			m_graceRecheckTimer.start();
			return;
		}
	}
	else
	{
		m_gracePeriod.remove(client, description);
	}

	switch (checkResult.access) // Flawfinder: ignore
	{
	case AccessControlProvider::Access::Allow:
		client->setAccessControlState(VncServerClient::AccessControlState::Successful);
		if (checkResult.matchedRule)
		{
			client->setAccessControlDetails(tr("Access allowed by rule \"%1\"").arg(checkResult.matchedRule->name()));
		}
		break;

	case AccessControlProvider::Access::ToBeConfirmed:
		client->setAccessControlState(confirmDesktopAccess(client));
		break;

	default:
		client->setAccessControlState(VncServerClient::AccessControlState::Failed);
		client->setProtocolState(VncServerProtocol::State::Close);
		if (checkResult.reason == AccessControlProvider::Reason::AccessControlRuleMatched && checkResult.matchedRule)
		{
			client->setAccessControlDetails(tr("Access denied by rule \"%1\"").arg(checkResult.matchedRule->name()));
		}
		else if (checkResult.reason == AccessControlProvider::Reason::NoAccessControlRuleMatched)
		{
			client->setAccessControlDetails(tr("No rule allowed access"));
		}
		else if (checkResult.reason == AccessControlProvider::Reason::UserNotInAuthorizedUserGroups)
		{
			client->setAccessControlDetails(tr("Accessing user not member of an authorized user group"));
		}
		else if (checkResult.reason == AccessControlProvider::Reason::DirectoryUnavailable)
		{
			client->setAccessControlDetails(tr("Directory for access control not available"));
		}
		break;
	}

	Q_EMIT finished(client);
}



VncServerClient::AccessControlState ServerAccessControlManager::confirmDesktopAccess(VncServerClient* client)
{
	const HostUserPair hostUserPair(client->username(), client->hostAddress());

	// did we save a previous choice because user chose "always" or "never"?
	if (m_desktopAccessChoices.contains(hostUserPair))
	{
		if (std::as_const(m_desktopAccessChoices)[hostUserPair] == DesktopAccessDialog::ChoiceAlways)
		{
			return VncServerClient::AccessControlState::Successful;
		}

		client->setAccessControlDetails(tr("User has denied access"));
		return VncServerClient::AccessControlState::Failed;
	}

	// already an access dialog running?
	if (m_desktopAccessDialog.isBusy(&m_featureWorkerManager))
	{
		// then close connection so that client has to try again later
		return VncServerClient::AccessControlState::Waiting;
	}

	// get notified whenever the dialog finishes - use signal indirection for
	// automatically breaking connection if VncServerClient gets deleted while
	// dialog is active (e.g. due another connection being closed and thus
	// all other connections are closed as well in order to perform access
	// control again)
	connect(&m_desktopAccessDialog, &DesktopAccessDialog::finished,
			client, &VncServerClient::finishAccessControl);

	connect(client, &VncServerClient::accessControlFinished,
			this, &ServerAccessControlManager::finishDesktopAccessConfirmation);

	// start the dialog (non-blocking)
	m_desktopAccessDialog.exec(&m_featureWorkerManager, client->username(), client->hostAddress());

	return VncServerClient::AccessControlState::Pending;
}



void ServerAccessControlManager::finishDesktopAccessConfirmation(VncServerClient* client)
{
	// break helper connections for asynchronous desktop access control operations
	if (m_desktopAccessDialog.disconnect(client) == false ||
		client->disconnect(this) == false)
	{
		vCritical() << "could not break object connections";
	}

	const auto choice = m_desktopAccessDialog.choice();

	// remember choices "always" and "never"
	if (choice == DesktopAccessDialog::ChoiceAlways || choice == DesktopAccessDialog::ChoiceNever)
	{
		m_desktopAccessChoices[HostUserPair(client->username(), client->hostAddress())] = choice;
	}

	// evaluate choice and set according access control state
	if (choice == DesktopAccessDialog::ChoiceYes || choice == DesktopAccessDialog::ChoiceAlways)
	{
		client->setAccessControlState(VncServerClient::AccessControlState::Successful);
		client->setAccessControlDetails(tr("User confirmed access"));
		m_clients.append(client);
	}
	else
	{
		client->setAccessControlState(VncServerClient::AccessControlState::Failed);
		client->setAccessControlDetails(tr("User has denied access"));
	}
}



QStringList ServerAccessControlManager::connectedUsers() const
{
	QStringList users;

	users.reserve(m_clients.size());

	for (auto client : m_clients)
	{
		users += client->username();
	}

	return users;
}
