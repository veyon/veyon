// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlatformSecretFunctions.h"
#include "SecretStore.h"


#if defined(VEYON_WITH_TESTS)
static PlatformSecretFunctions* testBackend = nullptr;
static VeyonCore::InitializationFlags testFlags = VeyonCore::InitializationFlag::SecretStore;
#endif


static bool isValidKeyComponent(QStringView component, int maximumLength, bool isScope)
{
	if (component.isEmpty() || component.size() > maximumLength)
	{
		return false;
	}

	for (const auto character : component)
	{
		const auto isLowerCaseLetterOrDigit = (character >= u'a' && character <= u'z') ||
			(character >= u'0' && character <= u'9');
		const auto isAllowed = isScope ? (isLowerCaseLetterOrDigit || character == u'-') : (isLowerCaseLetterOrDigit || (character >= u'A' && character <= u'Z') || character == u'.' || character == u'_' || character == u'-');
		if (isAllowed == false)
		{
			return false;
		}
	}

	return true;
}



bool SecretStore::isValidKey(const QString& key)
{
	const auto separatorIndex = key.indexOf(QLatin1Char('/'));
	if (separatorIndex < 0)
	{
		return false;
	}

	const auto scope = QStringView(key).left(separatorIndex);
	const auto name = QStringView(key).mid(separatorIndex + 1);

	// a trailing dot would be stripped from file names on Windows (and rules out "." and "..")
	return isValidKeyComponent(scope, MaximumScopeLength, true) &&
		isValidKeyComponent(name, MaximumNameLength, false) &&
		name.endsWith(u'.') == false;
}



bool SecretStore::isAvailable()
{
	return availableBackend() != nullptr;
}



std::optional<SecretStore::Secret> SecretStore::read(const QString& key)
{
	if (isValidKey(key) == false)
	{
		vWarning() << "invalid secret key";
		return {};
	}

	const auto backend = availableBackend();
	if (backend == nullptr)
	{
		return {};
	}

	auto secret = backend->readSecret(key);
	if (secret.has_value() && secret->size() > MaximumSecretSize)
	{
		return {};
	}

	return secret;
}



bool SecretStore::write(const QString& key, const Secret& value)
{
	if (isValidKey(key) == false)
	{
		vWarning() << "invalid secret key";
		return false;
	}

	if (value.size() > MaximumSecretSize)
	{
		vWarning() << "secret" << key << "too large";
		return false;
	}

	const auto backend = availableBackend();

	return backend && backend->writeSecret(key, value);
}



bool SecretStore::remove(const QString& key)
{
	if (isValidKey(key) == false)
	{
		vWarning() << "invalid secret key";
		return false;
	}

	const auto backend = availableBackend();

	return backend && backend->removeSecret(key);
}



QStringList SecretStore::keys()
{
	const auto backend = availableBackend();
	if (backend == nullptr)
	{
		return {};
	}

	QStringList validKeys;
	const auto backendKeys = backend->secretKeys();
	for (const auto& key : backendKeys)
	{
		if (isValidKey(key))
		{
			validKeys.append(key);
		}
	}

	return validKeys;
}



bool SecretStore::prepareStorage()
{
	const auto backend = availableBackend();

	return backend && backend->prepareStorage();
}



#if defined(VEYON_WITH_TESTS)
void SecretStore::setTestBackend(PlatformSecretFunctions* backend, VeyonCore::InitializationFlags flags)
{
	testBackend = backend;
	testFlags = flags;
}
#endif



PlatformSecretFunctions* SecretStore::availableBackend()
{
	PlatformSecretFunctions* backend = nullptr;
	VeyonCore::InitializationFlags flags;

#if defined(VEYON_WITH_TESTS)
	if (testBackend)
	{
		backend = testBackend;
		flags = testFlags;
	}
	else
#endif
		if (VeyonCore::instance())
	{
		backend = &VeyonCore::platform().secretFunctions();
		flags = VeyonCore::initializationFlags();
	}

	// veyon-server/veyon-worker run as root/SYSTEM on X11/Windows but get their secrets via IPC only
	// (03-secretstore-config.md §2), so they never get the SecretStore initialization flag
	if (backend == nullptr ||
		flags.testFlag(VeyonCore::InitializationFlag::SecretStore) == false ||
		backend->isAvailable() == false)
	{
		return nullptr;
	}

	return backend;
}
