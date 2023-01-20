// Copyright (c) 2019-2023 Tobias Junghans <tobydox@veyon.io>
// This file is part of Veyon - https://veyon.io
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "AuthLdapConfiguration.h"

class AuthLdapCore : public QObject
{
	Q_OBJECT
public:
	explicit AuthLdapCore( QObject* parent = nullptr );

	AuthLdapConfiguration& configuration()
	{
		return m_configuration;
	}

	void clear();

	const QString& username() const
	{
		return m_username;
	}

	void setUsername( const QString& username )
	{
		m_username = username;
	}

	const CryptoCore::PlaintextPassword& password() const
	{
		return m_password;
	}

	void setPassword( const CryptoCore::PlaintextPassword& password )
	{
		m_password = password;
	}

	bool hasCredentials() const;

	bool authenticate();

private:
	AuthLdapConfiguration m_configuration;

	QString m_username{};
	CryptoCore::PlaintextPassword m_password{};

};
