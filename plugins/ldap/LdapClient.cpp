// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QElapsedTimer>

#include "DirectoryBrokerLimits.h"
#include "DirectoryBrokerService.h"
#include "LdapClient.h"
#include "LdapConfiguration.h"
#include "SecretStore.h"
#include "ldapconnection.h"
#include "ldapcontrol.h"
#include "ldapoperation.h"
#include "ldapserver.h"

#include <ldap.h>


static inline KLDAPCore::LdapUrl::Scope kldapUrlScope(LdapClient::Scope scope)
{
	switch (scope)
	{
	case LdapClient::Scope::Base: return KLDAPCore::LdapUrl::Base;
	case LdapClient::Scope::One: return KLDAPCore::LdapUrl::One;
	case LdapClient::Scope::Sub: return KLDAPCore::LdapUrl::Sub;
	}

	return KLDAPCore::LdapUrl::Base;
}



LdapClient::LdapClient(const LdapConfiguration& configuration, const QUrl& url, QObject* parent) :
	QObject(parent),
	m_configuration(configuration),
	m_server(new KLDAPCore::LdapServer),
	m_connection(new KLDAPCore::LdapConnection),
	m_operation(new KLDAPCore::LdapOperation),
	m_queryTimeout(m_configuration.queryTimeout())
{
	connectAndBind(url);
}



LdapClient::~LdapClient()
{
	delete m_connection;
	delete m_operation;
	delete m_server;
}



QString LdapClient::errorString() const
{
	if (m_connection->handle() == nullptr)
	{
		return m_connection->connectionError();
	}

	return m_connection->ldapErrorString();
}



QString LdapClient::errorDescription() const
{
	const auto string = errorString();
	if (string.isEmpty() == false)
	{
		return tr("LDAP error description: %1").arg(string);
	}

	return {};
}



LdapClient::Objects LdapClient::queryObjects(const QString& dn, const QStringList& attributes,
											 const QString& filter, LdapClient::Scope scope)
{
	vDebug() << "called with" << dn << attributes << filter << scope;

	if (dn.isEmpty())
	{
		vCritical() << "DN is empty!";
		m_queryFailed = true;
		return {};
	}

	if (attributes.isEmpty())
	{
		vCritical() << "attributes empty!";
		m_queryFailed = true;
		return {};
	}

	Objects entries;

	auto realAttributeNames = attributes;
	for (auto& attribute : realAttributeNames)
	{
		attribute = attribute.toLower();
	}

	auto isFirstResult = true;

	search(dn, scope, filter, attributes, [&](const KLDAPCore::LdapObject& object) {
		if (isFirstResult)
		{
			isFirstResult = false;

			// match attribute name from result with requested attribute name in order
			// to keep result aggregation below case-insensitive
			const auto objectAttributes = object.attributes();
			for (auto it = objectAttributes.constBegin(), end = objectAttributes.constEnd(); it != end; ++it)
			{
				for (auto& attribute : realAttributeNames)
				{
					if (QString::compare(it.key().toLower(), attribute, Qt::CaseInsensitive) == 0)
					{
						attribute = it.key();
						break;
					}
				}
			}
		}

		// convert result list from type QList<QByteArray> to QStringList
		const auto objectDn = object.dn().toString();
		for (const auto& attribute : std::as_const(realAttributeNames))
		{
			const auto values = object.values(attribute);
			for (const auto& value : values)
			{
				entries[objectDn][attribute] += QString::fromUtf8(value);
			}
		}
	});

	vDebug() << "results:" << entries;

	return entries;
}



QStringList LdapClient::queryAttributeValues(const QString& dn, const QString& attribute,
											 const QString& filter, Scope scope)
{
	vDebug() << "called with" << dn << attribute << filter << scope;

	if (dn.isEmpty() && attribute != m_namingContextAttribute &&
		attribute.contains(QLatin1String("namingcontext"), Qt::CaseInsensitive) == false)
	{
		vCritical() << "DN is empty!";
		m_queryFailed = true;
		return {};
	}

	if (attribute.isEmpty())
	{
		vCritical() << "attribute is empty!";
		m_queryFailed = true;
		return {};
	}

	QStringList entries;

	bool isFirstResult = true;
	QString realAttributeName = attribute.toLower();

	search(dn, scope, filter, QStringList(attribute), [&](const KLDAPCore::LdapObject& object) {
		if (isFirstResult)
		{
			isFirstResult = false;

			// match attribute name from result with requested attribute name in order
			// to keep result aggregation below case-insensitive
			const auto attributes = object.attributes();
			for (auto it = attributes.constBegin(), end = attributes.constEnd(); it != end; ++it)
			{
				if (it.key().toLower() == realAttributeName)
				{
					realAttributeName = it.key();
					break;
				}
			}
		}

		// convert result list from type QList<QByteArray> to QStringList
		const auto values = object.values(realAttributeName);
		for (const auto& value : values)
		{
			entries += QString::fromUtf8(value);
		}
	});

	vDebug() << "results:" << entries;

	return entries;
}



QStringList LdapClient::queryDistinguishedNames(const QString& dn, const QString& filter, Scope scope)
{
	vDebug() << dn << filter << scope;

	if (dn.isEmpty())
	{
		vCritical() << "DN is empty!";
		m_queryFailed = true;
		return {};
	}

	QStringList distinguishedNames;

	search(dn, scope, filter, QStringList(), [&](const KLDAPCore::LdapObject& object) {
		distinguishedNames += object.dn().toString();
	});

	vDebug() << "results" << distinguishedNames;

	return distinguishedNames;
}



bool LdapClient::search(const QString& dn, Scope scope, const QString& filter, const QStringList& attributes,
						const EntryHandler& handleEntry)
{
	QList<KLDAPCore::LdapObject> entries;
	auto result = SearchResult::ConnectionError;

	// one retry with a new connection if the connection was lost
	for (int attempt = 0; attempt < 2 && result == SearchResult::ConnectionError; ++attempt)
	{
		entries.clear();

		// reconnecting and binding cannot be aborted, so do not start it for a cancelled request
		if (DirectoryBrokerService::isCurrentRequestCancelled())
		{
			result = SearchResult::Aborted;
			break;
		}

		if (m_state != Bound && reconnect() == false)
		{
			vCritical() << "not bound to server!";
			break;
		}

		result = runSearch(dn, scope, filter, attributes, entries);
		if (result == SearchResult::ConnectionError)
		{
			vWarning() << "LDAP search failed with code" << m_connection->ldapErrorCode() << "- reconnecting";
			m_state = Disconnected;
		}
		else if (result == SearchResult::Timeout)
		{
			// the connection may be dead without the socket noticing it (silently dropped by a
			// firewall): use a new one for the next query instead of waiting on this one again
			m_state = Disconnected;
		}
	}

	// partial results (e.g. size limit exceeded) are passed on, but the query counts as failed
	if (result == SearchResult::Success || result == SearchResult::ResultError)
	{
		for (const auto& entry : std::as_const(entries))
		{
			handleEntry(entry);
		}
	}

	if (result != SearchResult::Success)
	{
		m_queryFailed = true;
		return false;
	}

	return true;
}



LdapClient::SearchResult LdapClient::runSearch(const QString& dn, Scope scope, const QString& filter,
											   const QStringList& attributes, QList<KLDAPCore::LdapObject>& entries)
{
	QElapsedTimer searchTimer;
	searchTimer.start();

	// simple paged results (RFC 2696, non-critical: servers without support return all entries at
	// once), as AD returns at most MaxPageSize (default 1000) entries per search otherwise
	const auto savedControls = m_operation->serverControls();
	QByteArray cookie;

	for (;;)
	{
		if (DirectoryBrokerService::isCurrentRequestCancelled())
		{
			return SearchResult::Aborted;
		}

		auto controls = savedControls;
		KLDAPCore::LdapControl::insert(controls, KLDAPCore::LdapControl::createPageControl(PageSize, cookie));
		m_operation->setServerControls(controls);
		const auto id = m_operation->search(KLDAPCore::LdapDN(dn), kldapUrlScope(scope), filter, attributes);
		m_operation->setServerControls(savedControls);

		if (id == -1)
		{
			return SearchResult::ConnectionError;
		}

		const auto result = receiveSearchResults(id, searchTimer, entries);
		if (result != SearchResult::Success)
		{
			return result;
		}

		cookie.clear();
		const auto resultControls = m_operation->controls();
		for (const auto& control : resultControls)
		{
			if (control.parsePageControl(cookie) != -1)
			{
				break;
			}
		}

		if (cookie.isEmpty())
		{
			return SearchResult::Success;
		}
	}
}



LdapClient::SearchResult LdapClient::receiveSearchResults(int id, const QElapsedTimer& searchTimer,
														  QList<KLDAPCore::LdapObject>& entries)
{
	// the query timeout applies to the time without any response from the server, the whole search
	// (all pages) is limited by MaximumSearchDuration
	QElapsedTimer inactivityTimer;
	inactivityTimer.start();

	for (;;)
	{
		if (DirectoryBrokerService::isCurrentRequestCancelled())
		{
			vWarning() << "aborting LDAP search of cancelled request";
			(void)m_operation->abandon(id);
			return SearchResult::Aborted;
		}

		const auto remainingTime = qMin<qint64>(m_queryTimeout - inactivityTimer.elapsed(),
												MaximumSearchDuration - searchTimer.elapsed());
		if (remainingTime <= 0)
		{
			vWarning() << "LDAP search timed out";
			(void)m_operation->abandon(id);
			return SearchResult::Timeout;
		}

		const auto waitTime = int(qMin<qint64>(remainingTime, AbortCheckInterval));
		QElapsedTimer waitTimer;
		waitTimer.start();

		const auto result = m_operation->waitForResult(id, waitTime);
		switch (result)
		{
		case KLDAPCore::LdapOperation::RES_SEARCH_ENTRY:
			inactivityTimer.restart();
			entries.append(m_operation->object());
			if (entries.size() > DirectoryBrokerLimits::MaximumResultItems)
			{
				vWarning() << "LDAP search returned too many entries";
				(void)m_operation->abandon(id);
				return SearchResult::ResultError;
			}
			break;
		case 0:
			// returned early: a search reference (referrals are not chased), else no response
			// within this interval
			if (waitTimer.elapsed() < waitTime)
			{
				inactivityTimer.restart();
			}
			break;
		case KLDAPCore::LdapOperation::RES_SEARCH_RESULT:
		{
			// libldap stores the result code of the parsed search result
			const auto resultCode = m_connection->ldapErrorCode();
			if (resultCode == LDAP_SUCCESS)
			{
				return SearchResult::Success;
			}
			if (resultCode == LDAP_SIZELIMIT_EXCEEDED)
			{
				vWarning() << "LDAP search exceeded the size limit of the server (e.g. AD's MaxPageSize "
							  "without support for paged results) or of Veyon ("
						   << DirectoryBrokerLimits::MaximumResultItems << "entries)";
			}
			else
			{
				vWarning() << "LDAP search completed with result code" << resultCode << errorString();
			}
			return SearchResult::ResultError;
		}
		case -1:
			return SearchResult::ConnectionError;
		default:
			vWarning() << "unexpected LDAP result type" << result;
			(void)m_operation->abandon(id);
			return SearchResult::ResultError;
		}
	}
}



QStringList LdapClient::queryObjectAttributes(const QString& dn)
{
	vDebug() << "called with" << dn;

	if (m_state != Bound && reconnect() == false)
	{
		vCritical() << "not bound to server!";
		return {};
	}

	if (dn.isEmpty())
	{
		vCritical() << "DN is empty!";
		return {};
	}

	int id = 0;
	if (ldap_search_ext(static_cast<LDAP*>(m_connection->handle()),
						dn.toUtf8().data(), LDAP_SCOPE_BASE, "objectClass=*",
						nullptr, 1, nullptr, nullptr, nullptr,
						m_connection->sizeLimit(), &id) != 0)
	{
		return {};
	}

	if (m_operation->waitForResult(id, m_queryTimeout) == KLDAPCore::LdapOperation::RES_SEARCH_ENTRY)
	{
		const auto keys = m_operation->object().attributes().keys();
		vDebug() << "results" << keys;
		return keys;
	}

	return {};
}



QStringList LdapClient::queryBaseDn()
{
	return queryDistinguishedNames(baseDn(), QStringLiteral("(objectclass=*)"), Scope::Base);
}



QStringList LdapClient::queryNamingContexts(const QString& attribute)
{
	return queryAttributeValues({}, attribute.isEmpty() ? m_namingContextAttribute : attribute);
}



QString LdapClient::baseDn()
{
	if (m_baseDn.isEmpty())
	{
		// query base DN via naming context if configured
		if (m_configuration.queryNamingContext())
		{
			m_baseDn = queryNamingContexts().value(0);
		}
		else
		{
			// use the configured base DN
			m_baseDn = m_configuration.baseDn();
		}
	}

	return m_baseDn;
}



QString LdapClient::parentDn(const QString& dn)
{
	auto rdns = toRDNs(dn);
	if (rdns.size() > 1)
	{
		return rdns.mid(1).join(QLatin1Char(','));
	}

	return {};
}



QString LdapClient::stripBaseDn(const QString& dn, const QString& baseDn)
{
	const auto fullDnLower = dn.toLower();
	const auto baseDnLower = baseDn.toLower();

	if (fullDnLower.endsWith(QLatin1Char(',') + baseDnLower) && dn.length() > baseDn.length() + 1)
	{
		// cut off comma and base DN
		return dn.left(dn.length() - baseDn.length() - 1);
	}
	else if (fullDnLower == baseDnLower)
	{
		return {};
	}

	return dn;
}



QString LdapClient::addBaseDn(const QString& relativeDn, const QString& baseDn)
{
	if (relativeDn.isEmpty())
	{
		return baseDn;
	}

	return relativeDn + QLatin1Char(',') + baseDn;
}



QStringList LdapClient::stripBaseDn(const QStringList& dns, const QString& baseDn)
{
	QStringList strippedDns;

	strippedDns.reserve(dns.size());

	for (const auto& dn : dns)
	{
		strippedDns += stripBaseDn(dn, baseDn);
	}

	return strippedDns;
}



bool LdapClient::connectAndBind(const QUrl& url)
{
	if (url.isValid())
	{
		m_server->setUrl(KLDAPCore::LdapUrl(url));
	}
	else
	{
		m_server->setHost(m_configuration.serverHost());
		m_server->setPort(m_configuration.serverPort());

		if (m_configuration.useBindCredentials())
		{
			m_server->setBindDn(m_configuration.bindDn());
			m_server->setAuth(KLDAPCore::LdapServer::Simple);
			// without the password reconnect() refuses to connect (and tries again later)
			(void)loadBindPassword();
		}
		else
		{
			m_server->setAuth(KLDAPCore::LdapServer::Anonymous);
		}

		const auto security = static_cast<ConnectionSecurity>(m_configuration.connectionSecurity());
		switch (security)
		{
		case ConnectionSecurityTLS:
			m_server->setSecurity(KLDAPCore::LdapServer::TLS);
			break;
		case ConnectionSecuritySSL:
			m_server->setSecurity(KLDAPCore::LdapServer::SSL);
			break;
		default:
			m_server->setSecurity(KLDAPCore::LdapServer::None);
			break;
		}
	}

	if (m_configuration.connectionSecurity() != ConnectionSecurityNone)
	{
		initTLS();
	}

	// server side limits and the timeout of synchronous operations (bind)
	const auto timeLimit = qMax(1, (m_queryTimeout + 999) / 1000);
	m_server->setTimeout(timeLimit);
	m_server->setTimeLimit(timeLimit);
	m_server->setSizeLimit(DirectoryBrokerLimits::MaximumResultItems);

	// also needed if connecting fails now and succeeds later
	m_namingContextAttribute = m_configuration.namingContextAttribute();

	if (m_namingContextAttribute.isEmpty())
	{
		// fallback to AD default value
		m_namingContextAttribute = QStringLiteral("defaultNamingContext");
	}

	return reconnect();
}



bool LdapClient::loadBindPassword()
{
	// available in privileged components only (service, elevated CLI/configurator); veyon-server
	// and the master use the directory broker instead (04-directory-broker.md)
	const auto bindPassword = SecretStore::read(m_configuration.bindPassword().key());

	// a simple bind with a DN but an empty password is an unauthenticated bind (RFC 4513 §5.1.2)
	// which many servers accept anonymously: never fall back to it
	if (bindPassword.has_value() == false || bindPassword->isEmpty())
	{
		vCritical() << "LDAP bind password not available in this process - not connecting";
		m_bindCredentialsUnavailable = true;
		m_bindCredentialsRetryTimer.start();
		return false;
	}

	m_server->setPassword(QString::fromUtf8(bindPassword->constData(), bindPassword->size()));
	m_bindCredentialsUnavailable = false;

	return true;
}



void LdapClient::initTLS()
{
	switch (m_configuration.tlsVerifyMode())
	{
	case TLSVerifyDefault:
		m_server->setTLSRequireCertificate(KLDAPCore::LdapServer::TLSReqCertDefault);
		break;
	case TLSVerifyNever:
		m_server->setTLSRequireCertificate(KLDAPCore::LdapServer::TLSReqCertNever);
		break;
	case TLSVerifyCustomCert:
		m_server->setTLSRequireCertificate(KLDAPCore::LdapServer::TLSReqCertHard);
		m_server->setTLSCACertFile(m_configuration.tlsCACertificateFile());
		break;
	default:
		vCritical() << "invalid TLS verify mode specified!";
		m_server->setTLSRequireCertificate(KLDAPCore::LdapServer::TLSReqCertDefault);
		break;
	}
}



bool LdapClient::reconnect()
{
	m_connection->close();
	m_state = Disconnected;

	// the bind password may become available later (e.g. after an import), but never fall back to
	// connecting without it
	if (m_bindCredentialsUnavailable &&
		(m_bindCredentialsRetryTimer.hasExpired(m_bindCredentialsRetryInterval) == false || loadBindPassword() == false))
	{
		return false;
	}

	m_connection->setServer(*m_server);

	if (qEnvironmentVariableIsSet("VEYON_DEBUG_LDAP_LIBRARY"))
	{
		const auto debugLevel = LdapLibraryDebugAny;
		ldap_set_option(nullptr, LDAP_OPT_DEBUG_LEVEL, &debugLevel);
		ber_set_option(nullptr, LDAP_OPT_DEBUG_LEVEL, &debugLevel);
	}

	if (m_connection->connect() != 0)
	{
		vWarning() << "LDAP connect failed:" << errorString();
		return false;
	}

	const auto ldap = static_cast<LDAP*>(m_connection->handle());

	// never chase referrals/search references: libldap would follow them anonymously to hosts
	// named by the server (from the root service), and AD answers such chased searches (e.g.
	// DomainDnsZones below the domain root) with operationsError, failing the whole query
	if (ldap_set_option(ldap, LDAP_OPT_REFERRALS, LDAP_OPT_OFF) != LDAP_OPT_SUCCESS)
	{
		vWarning() << "failed to disable LDAP referral chasing";
	}

#if defined(LDAP_OPT_X_KEEPALIVE_IDLE) && defined(LDAP_OPT_X_KEEPALIVE_PROBES) && defined(LDAP_OPT_X_KEEPALIVE_INTERVAL)
	// detect silently dropped connections
	const int keepAliveIdle = KeepAliveIdle;
	const int keepAliveProbes = KeepAliveProbes;
	const int keepAliveInterval = KeepAliveInterval;
	if (ldap_set_option(ldap, LDAP_OPT_X_KEEPALIVE_IDLE, &keepAliveIdle) != LDAP_OPT_SUCCESS ||
		ldap_set_option(ldap, LDAP_OPT_X_KEEPALIVE_PROBES, &keepAliveProbes) != LDAP_OPT_SUCCESS ||
		ldap_set_option(ldap, LDAP_OPT_X_KEEPALIVE_INTERVAL, &keepAliveInterval) != LDAP_OPT_SUCCESS)
	{
		vWarning() << "failed to set LDAP keep-alive options";
	}
#endif

	// libldap connects with the first operation (bind): never wait for an unreachable server
	// longer than for a query
	struct timeval networkTimeout{};
	networkTimeout.tv_sec = m_queryTimeout / 1000;
	networkTimeout.tv_usec = (m_queryTimeout % 1000) * 1000;
	if (ldap_set_option(ldap, LDAP_OPT_NETWORK_TIMEOUT, &networkTimeout) != LDAP_OPT_SUCCESS)
	{
		vWarning() << "failed to set LDAP network timeout";
	}

	m_state = Connected;

	m_operation->setConnection(*m_connection);
	if (m_operation->bind_s() != 0)
	{
		vWarning() << "LDAP bind failed:" << errorString();
		return false;
	}

	m_state = Bound;

	return true;
}



QString LdapClient::constructSubDn(const QString& subtree, const QString& baseDn)
{
	if (baseDn.isEmpty())
	{
		return {};
	}

	if (subtree.isEmpty())
	{
		return baseDn;
	}

	return subtree + QLatin1Char(',') + baseDn;
}



QString LdapClient::constructQueryFilter(const QString& filterAttribute,
										 const QString& filterValue,
										 const QString& extraFilter)
{
	QString queryFilter;

	if (filterAttribute.isEmpty() == false)
	{
		if (filterValue.isEmpty())
		{
			queryFilter = QStringLiteral("(%1=*)").arg(filterAttribute);
		}
		else
		{
			queryFilter = QStringLiteral("(%1=%2)").arg(filterAttribute,
														escapeFilterValue(filterValue));
		}
	}

	if (extraFilter.isEmpty() == false)
	{
		if (queryFilter.isEmpty())
		{
			queryFilter = extraFilter;
		}
		else
		{
			queryFilter = QStringLiteral("(&%1%2)").arg(extraFilter, queryFilter);
		}
	}

	return queryFilter;
}



QString LdapClient::escapeFilterValue(const QString& filterValue)
{
	return QString(filterValue)
		.replace(QStringLiteral("\\"), QStringLiteral("\\5c"))
		.replace(QStringLiteral("("), QStringLiteral("\\28"))
		.replace(QStringLiteral(")"), QStringLiteral("\\29"))
		.replace(QStringLiteral("*"), QStringLiteral("\\2a"))
		.replace(QChar(QChar::Null), QStringLiteral("\\00"));
}



QStringList LdapClient::toRDNs(const QString& dn)
{
	QStringList strParts;
	int index = 0;
	int searchFrom = 0;
	int strPartStartIndex = 0;
	while ((index = dn.indexOf(QLatin1Char(','), searchFrom)) != -1)
	{
		const auto prev = dn[std::max(0, index - 1)];
		if (prev != QLatin1Char('\\'))
		{
			strParts.append(dn.mid(strPartStartIndex, index - strPartStartIndex));
			strPartStartIndex = index + 1;
		}

		searchFrom = index + 1;
	}

	strParts.append(dn.mid(strPartStartIndex));

	return strParts;
}
