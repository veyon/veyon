// Copyright (c) 2019-2024 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QObject>
#include <QUrl>

#include "LdapCommon.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
namespace KLDAP {
class LdapConnection;
class LdapOperation;
class LdapServer;
}
namespace KLDAPCore = KLDAP;
#else
namespace KLDAPCore {
class LdapConnection;
class LdapOperation;
class LdapServer;
}
#endif

class LdapConfiguration;

class LDAP_COMMON_EXPORT LdapClient : public QObject
{
	Q_OBJECT
public:
	enum class Scope {
		Base,
		One,
		Sub
	};
	Q_ENUM(Scope)

	enum ConnectionSecurity
	{
		ConnectionSecurityNone,
		ConnectionSecurityTLS,
		ConnectionSecuritySSL,
		ConnectionSecurityCount,
	};
	Q_ENUM(ConnectionSecurity)

	enum TLSVerifyMode
	{
		TLSVerifyDefault,
		TLSVerifyNever,
		TLSVerifyCustomCert,
		TLSVerifyModeCount
	};
	Q_ENUM(TLSVerifyMode)

	using Objects = QMap<QString, QMap<QString, QStringList> >;

	explicit LdapClient( const LdapConfiguration& configuration, const QUrl& url = QUrl(), QObject* parent = nullptr );
	~LdapClient() override;

	const LdapConfiguration& configuration() const
	{
		return m_configuration;
	}

	bool isConnected() const
	{
		return m_state >= Connected;
	}

	bool isBound() const
	{
		return m_state >= Bound;
	}

	QString errorString() const;
	QString errorDescription() const;

	Objects queryObjects( const QString& dn, const QStringList& attributes, const QString& filter, Scope scope );

	QStringList queryAttributeValues( const QString &dn, const QString &attribute,
									  const QString& filter = QStringLiteral( "(objectclass=*)" ),
									  Scope scope = Scope::Base );

	QStringList queryDistinguishedNames( const QString& dn, const QString& filter, Scope scope );

	QStringList queryObjectAttributes( const QString& dn );

	QStringList queryBaseDn();

	QStringList queryNamingContexts( const QString& attribute = {} );

	QString baseDn();

	static QString parentDn( const QString& dn );
	static QString stripBaseDn( const QString& dn, const QString& baseDn );
	static QString addBaseDn( const QString& rdns, const QString& baseDn );

	static QStringList stripBaseDn( const QStringList& dns, const QString& baseDn );

	static QString constructSubDn( const QString& subtree, const QString& baseDn );

	static QString constructQueryFilter( const QString& filterAttribute,
										 const QString& filterValue,
										 const QString& extraFilter = {} );

	static QString escapeFilterValue( const QString& filterValue );

	static QStringList toRDNs( const QString& dn );

	static QString cn()
	{
		return QStringLiteral("cn");
	}

	static constexpr int DefaultQueryTimeout = 3000;

private:
	static constexpr auto LdapLibraryDebugAny = -1;

	bool reconnect();
	bool connectAndBind( const QUrl& url );
	void initTLS();

	const LdapConfiguration& m_configuration;
	KLDAPCore::LdapServer* m_server;
	KLDAPCore::LdapConnection* m_connection;
	KLDAPCore::LdapOperation* m_operation;

	enum State
	{
		Disconnected,
		Connected,
		Bound,
		StateCount
	} ;

	State m_state = Disconnected;

	bool m_queryRetry = false;

	QString m_baseDn;
	QString m_namingContextAttribute;

	const int m_queryTimeout{DefaultQueryTimeout};

};
