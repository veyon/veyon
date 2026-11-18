// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>

#include "AuthenticationCredentials.h"
#include "CryptoCore.h"
#include "VeyonConnection.h"

#include <cstring>

// The server side of a master connection: reads come from the fuzzer input, writes are discarded.
class FuzzedServerDevice : public QIODevice
{
public:
	FuzzedServerDevice(const char* data, qint64 size) :
		m_data(data),
		m_size(size)
	{
		open(QIODevice::ReadWrite | QIODevice::Unbuffered);
	}

	bool isSequential() const override
	{
		return true;
	}

protected:
	qint64 readData(char* data, qint64 maxSize) override
	{
		const auto size = std::min(maxSize, m_size - m_position);
		if (size > 0)
		{
			std::memcpy(data, m_data + m_position, size_t(size));
		}
		m_position += size;
		return size;
	}

	qint64 writeData(const char* data, qint64 size) override
	{
		Q_UNUSED(data)
		return size;
	}

private:
	const char* m_data;
	const qint64 m_size;
	qint64 m_position = 0;
};



static VeyonConnection* connection = nullptr;



static void discardLogMessage(QtMsgType, const QMessageLogContext&, const QString&)
{
}



extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
	auto app = new QCoreApplication(*argc, *argv);

	// never read or write a real configuration
	static QTemporaryDir configurationDirectory;
	QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope, configurationDirectory.filePath(QStringLiteral("system")));
	QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, configurationDirectory.filePath(QStringLiteral("user")));

	new VeyonCore(app, VeyonCore::Component::Master, QStringLiteral("Fuzzer"),
				  VeyonCore::InitializationFlags(VeyonCore::InitializationFlag::LegacyCrypto) |
					  VeyonCore::InitializationFlag::AuthenticationCredentials);

	auto& credentials = VeyonCore::authenticationCredentials();
	credentials.setLogonUsername(QStringLiteral("teacher"));
	credentials.setLogonPassword(AuthenticationCredentials::Password(QByteArray("password")));
	credentials.setPrivateKey(CryptoCore::KeyGenerator().createRSA(CryptoCore::RsaKeySize));
	credentials.setAuthenticationKeyName(QStringLiteral("teacher"));
	credentials.setToken(AuthenticationCredentials::Token(QByteArray(CryptoCore::ChallengeSize, 't')));

	qInstallMessageHandler(discardLogMessage);

	// authenticate() only reads from the connection, so one instance serves all inputs
	// (a VncConnection per input grows the RSS by several KB per run)
	connection = new VeyonConnection;

	return 0;
}



// pre-auth parsing on the master (05-tls.md §6): the authentication types and all authentication
// messages of an unauthenticated server; the first byte selects the configured authentication type
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	if (size < 1)
	{
		return 0;
	}

	static constexpr RfbVeyonAuth::Type AuthTypes[] = {RfbVeyonAuth::KeyFile, RfbVeyonAuth::Logon, RfbVeyonAuth::Token};

	connection->setVeyonAuthType(AuthTypes[static_cast<unsigned char>(data[0]) % 3]);

	FuzzedServerDevice device(data + 1, qint64(size - 1));
	(void)VeyonConnection::authenticate(connection, device);

	return 0;
}
