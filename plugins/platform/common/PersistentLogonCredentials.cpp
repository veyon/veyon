// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QtEndian>

#include "IpcClient.h"
#include "PersistentLogonCredentials.h"

#include <cstring>

using Veyon::Crypto::SecureBuffer;

static constexpr qsizetype FieldSizeSize = sizeof(quint32);


// Copies of the data outside SecureBuffers are wiped as soon as they are no longer needed. Not
// covered: the base64 strings inside the IPC messages (QJsonObject), which are only sent over the
// encrypted IPC channel.

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

	auto encodedData = response.result.value(IpcProtocol::Key::Data).toString().toLatin1();
	auto decodedData = QByteArray::fromBase64Encoding(encodedData, QByteArray::AbortOnBase64DecodingErrors);
	encodedData.fill(0);
	const auto logonData = SecureBuffer::fromByteArray(decodedData.decoded);
	decodedData.decoded.fill(0);

	if (response.ok == false || decodedData.decodingStatus != QByteArray::Base64DecodingStatus::Ok ||
		logonData.isEmpty())
	{
		vDebug() << "no logon credentials available";
		return false;
	}

	if (decode(logonData, username, password) == false)
	{
		vWarning() << "invalid logon credentials received";
		return false;
	}

	return true;
}



bool PersistentLogonCredentials::write(const QString& username, const Password& password)
{
	const auto logonData = encode(username, password);
	if (logonData.has_value() == false)
	{
		vCritical() << "invalid logon credentials";
		return false;
	}

	const auto ipcClient = VeyonCore::serviceIpcClient();
	if (ipcClient == nullptr)
	{
		vCritical() << "no connection to service";
		return false;
	}

	auto encodedData = QByteArray::fromRawData(logonData->constData(), logonData->size()).toBase64();
	auto encodedString = QString::fromLatin1(encodedData);
	encodedData.fill(0);

	const auto success = ipcClient->call(IpcProtocol::Command::SetSessionData,
										 {{IpcProtocol::Key::Data, encodedString}})
							 .ok;
	encodedString.fill(QChar());

	if (success == false)
	{
		vCritical() << "Failed to write persistent service data";
		return false;
	}

	return true;
}



std::optional<SecureBuffer> PersistentLogonCredentials::encode(const QString& username, const Password& password)
{
	const auto usernameData = username.toUtf8();
	if (usernameData.isEmpty() || usernameData.size() > MaximumUsernameSize ||
		password.isEmpty() || password.size() > MaximumPasswordSize)
	{
		return {};
	}

	SecureBuffer data(FieldSizeSize + usernameData.size() + FieldSizeSize + password.size());
	auto output = data.data();

	qToBigEndian<quint32>(quint32(usernameData.size()), output);
	output += FieldSizeSize;
	std::memcpy(output, usernameData.constData(), size_t(usernameData.size()));
	output += usernameData.size();

	qToBigEndian<quint32>(quint32(password.size()), output);
	output += FieldSizeSize;
	std::memcpy(output, password.constData(), size_t(password.size()));

	return data;
}



bool PersistentLogonCredentials::decode(const SecureBuffer& data, QString* username, Password* password)
{
	if (username == nullptr || password == nullptr)
	{
		return false;
	}

	qsizetype offset = 0;

	// returns the size of the field at offset and moves offset to its data, or 0 if invalid
	const auto nextField = [&data, &offset](qsizetype maximumSize) -> qsizetype {
		if (data.size() - offset < FieldSizeSize)
		{
			return 0;
		}

		const auto size = qsizetype(qFromBigEndian<quint32>(data.constData() + offset));
		offset += FieldSizeSize;

		return size <= maximumSize && size <= data.size() - offset ? size : 0;
	};

	const auto usernameSize = nextField(MaximumUsernameSize);
	if (usernameSize == 0)
	{
		return false;
	}

	const auto usernameOffset = offset;
	offset += usernameSize;

	const auto passwordSize = nextField(MaximumPasswordSize);
	if (passwordSize == 0 || offset + passwordSize != data.size())
	{
		return false;
	}

	*username = QString::fromUtf8(data.constData() + usernameOffset, usernameSize);

	*password = Password(int(passwordSize));
	std::memcpy(password->data(), data.constData() + offset, size_t(passwordSize));

	return true;
}
