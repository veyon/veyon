// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QBuffer>

#include "IpcClient.h"
#include "PersistentLogonCredentials.h"
#include "VariantStream.h"


bool PersistentLogonCredentials::read(QString* username, Password* password)
{
	if (username == nullptr || password == nullptr)
	{
		vCritical() << "Invalid input pointers";
		return false;
	}

	const auto ipcClient = VeyonCore::serviceIpcClient();
	if (ipcClient == nullptr)
	{
		vCritical() << "no connection to service";
		return false;
	}

	const auto response = ipcClient->call(IpcProtocol::Command::GetSessionData);
	auto logonData = QByteArray::fromBase64(response.result.value(IpcProtocol::Key::Data).toString().toLatin1());
	if (response.ok == false || logonData.isEmpty())
	{
		vDebug() << "no logon credentials available";
		return false;
	}

	QBuffer logonDataBuffer(&logonData);
	if (logonDataBuffer.open(QBuffer::ReadOnly) == false)
	{
		vCritical() << "Failed to open buffer";
		return false;
	}

	// the password is stored in plain text: the blob only travels over the authenticated and
	// encrypted IPC channel and lives in the service's memory (02-ipc.md §6, 05-tls.md §7)
	VariantStream stream(&logonDataBuffer);
	*username = stream.read().toString();
	auto passwordData = stream.read();
	if (passwordData.userType() == QMetaType::QByteArray)
	{
		auto plainTextPassword = passwordData.toByteArray();
		// release the variant's reference first so that fill() wipes the only copy in place
		passwordData.clear();
		*password = Password(plainTextPassword);
		plainTextPassword.fill(0);
	}
	logonData.fill(0);

	return username->isEmpty() == false && password->isEmpty() == false;
}



bool PersistentLogonCredentials::write(const QString& username, const Password& password)
{
	QBuffer logonDataBuffer;
	if (logonDataBuffer.open(QBuffer::WriteOnly) == false)
	{
		vCritical() << "Failed to open buffer";
		return false;
	}

	VariantStream stream(&logonDataBuffer);
	stream.write(username);
	auto plainTextPassword = QByteArray(password.constData(), password.size());
	stream.write(plainTextPassword);
	plainTextPassword.fill(0);

	const auto ipcClient = VeyonCore::serviceIpcClient();
	const auto success = ipcClient &&
		ipcClient->call(IpcProtocol::Command::SetSessionData,
						{{IpcProtocol::Key::Data, QString::fromLatin1(logonDataBuffer.data().toBase64())}})
			.ok;
	logonDataBuffer.buffer().fill(0);

	if (success == false)
	{
		vCritical() << "Failed to write persistent service data";
		return false;
	}

	return true;
}
