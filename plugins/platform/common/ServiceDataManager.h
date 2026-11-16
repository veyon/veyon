// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

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

	ServiceDataManager(QObject* parent = nullptr);
	~ServiceDataManager() override;

	const Token& token() const
	{
		return m_token;
	}

	static QByteArray read(const Token& token);
	static bool write(const Token& token, const Data& data);

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

	static bool waitForMessage(QLocalSocket* socket);
	static void sendMessage(QLocalSocket* socket, VariantArrayMessage& message);

	void acceptConnection();
	void handleConnection(QLocalSocket* socket);

	static QString serverName()
	{
		return QStringLiteral("VeyonServiceDataManager");
	}

	QLocalServer* m_server{nullptr};

	Token m_token;
	Data m_data;
};
