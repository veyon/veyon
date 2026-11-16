// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QBuffer>

#include "PersistentLogonCredentials.h"
#include "ServiceDataManager.h"
#include "VariantStream.h"


bool PersistentLogonCredentials::read(QString* username, Password* password)
{
	if (username == nullptr || password == nullptr)
	{
		vCritical() << "Invalid input pointers";
		return false;
	}

	auto logonData = ServiceDataManager::read(ServiceDataManager::serviceDataTokenFromEnvironment());
	if (logonData.isEmpty())
	{
		vCritical() << "Empty data";
		return false;
	}

	QBuffer logonDataBuffer(&logonData);
	if (logonDataBuffer.open(QBuffer::ReadOnly) == false)
	{
		vCritical() << "Failed to open buffer";
		return false;
	}

	VariantStream stream(&logonDataBuffer);
	*username = stream.read().toString();
	*password = VeyonCore::cryptoCore().decryptPassword(stream.read().toString());

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
	stream.write(VeyonCore::cryptoCore().encryptPassword(password));

	if (ServiceDataManager::write(ServiceDataManager::serviceDataTokenFromEnvironment(),
								  logonDataBuffer.data()) == false)
	{
		vCritical() << "Failed to write persistent service data";
		return false;
	}

	return true;
}



bool PersistentLogonCredentials::clear()
{
	return ServiceDataManager::write(ServiceDataManager::serviceDataTokenFromEnvironment(), {});
}
