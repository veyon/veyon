/*
 * ServiceDataManager.h - header file for ServiceDataManager class
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QThread>

#include "CryptoCore.h"
#include "VariantArrayMessage.h"

class QLocalServer;
class QLocalSocket;

class ServiceDataManager : public QThread
{
	Q_OBJECT
public:
	enum class Command {
		NoCommand,
		ReadData,
		WriteData,
	};
	Q_ENUM(Command)

	using Data = CryptoCore::PlaintextPassword;
	using Token = CryptoCore::PlaintextPassword;

	ServiceDataManager( QObject* parent = nullptr );
	~ServiceDataManager() override;

	const Token& token() const
	{
		return m_token;
	}

	static QByteArray read( const Token& token );
	static bool write( const Token& token, const Data& data );

	static const char* serviceDataTokenEnvironmentVariable()
	{
		return "VEYON_SERVICE_DATA_TOKEN";
	}

	static Token serviceDataTokenFromEnvironment();

protected:
	void run() override;

private:
	static constexpr auto SocketWaitTimeout = 1000;
	static constexpr auto MessageReadTimeout = 10000;

	static bool waitForMessage( QLocalSocket* socket );
	static void sendMessage( QLocalSocket* socket, VariantArrayMessage& message );

	void acceptConnection();
	void handleConnection( QLocalSocket* socket );

	static QString serverName()
	{
		return QStringLiteral("VeyonServiceDataManager");
	}

	QLocalServer* m_server{nullptr};

	Token m_token;
	Data m_data;

};
