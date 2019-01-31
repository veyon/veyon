/*
 * LdapClient.h - class implementing an LDAP client
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QObject>
#include <QUrl>

namespace KLDAP {
class LdapConnection;
class LdapOperation;
class LdapServer;
}

class LdapConfiguration;

class LdapClient : public QObject
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

	typedef QMap<QString, QMap<QString, QStringList> > Objects;

	LdapClient( const LdapConfiguration& configuration, const QUrl& url = QUrl(), QObject* parent = nullptr );
	~LdapClient();

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

	QStringList queryNamingContexts( const QString& attribute = QString() );

	const QString& baseDn() const
	{
		return m_baseDn;
	}

	static QString parentDn( const QString& dn );
	static QString stripBaseDn( const QString& dn, const QString& baseDn );
	static QString addBaseDn( const QString& rdns, const QString& baseDn );

	static QStringList stripBaseDn( const QStringList& dns, const QString& baseDn );

	static QString constructSubDn( const QString& subtree, const QString& baseDn );

	static QString constructQueryFilter( const QString& filterAttribute,
										 const QString& filterValue,
										 const QString& extraFilter = QString() );

	static QString escapeFilterValue( const QString& filterValue );

	static QStringList toRDNs( const QString& dn );

private:
	static constexpr int LdapQueryTimeout = 3000;
	static constexpr int LdapConnectionTimeout = 60*1000;

	bool reconnect();
	bool connectAndBind( const QUrl& url );
	void initTLS();

	const LdapConfiguration& m_configuration;
	KLDAP::LdapServer* m_server;
	KLDAP::LdapConnection* m_connection;
	KLDAP::LdapOperation* m_operation;

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

};
