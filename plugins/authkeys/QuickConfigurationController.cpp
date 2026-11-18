// SPDX-License-Identifier: GPL-2.0-or-later
#include "QuickConfigurationController.h"
#include "AuthKeysManager.h"
#include "Filesystem.h"
#include <QDir>

AuthKeysQuickConfigurationController::AuthKeysQuickConfigurationController(VeyonConfiguration* draft, QObject* parent)
	: QObject(parent), m_draft(draft)
{
	setObjectName(QStringLiteral("keys"));
	connect(draft, &Configuration::Object::configurationChanged, this, &AuthKeysQuickConfigurationController::changed);
}
QVariantList AuthKeysQuickConfigurationController::keys() const
{
	AuthKeysManager manager(*m_draft); QVariantList result;
	for (const auto& key : manager.listKeys()) result.append(QVariantMap{{QStringLiteral("key"), key}, {QStringLiteral("name"), key.section(QLatin1Char('/'), 0, 0)}, {QStringLiteral("type"), key.section(QLatin1Char('/'), 1, 1)}, {QStringLiteral("group"), manager.accessGroup(key)}, {QStringLiteral("pairId"), manager.keyPairId(key)}});
	return result;
}
bool AuthKeysQuickConfigurationController::finish(bool success, const QString& message)
{
	m_message = message.isEmpty() ? (success ? tr("Operation completed.") : tr("Operation failed.")) : message;
	Q_EMIT changed(); return success;
}
bool AuthKeysQuickConfigurationController::create(const QString& name)
{
	AuthKeysManager manager(*m_draft); auto ok = manager.createKeyPair(name); return finish(ok, manager.resultMessage());
}
bool AuthKeysQuickConfigurationController::remove(const QString& key)
{
	AuthKeysManager manager(*m_draft); auto ok = manager.deleteKey(key.section(QLatin1Char('/'), 0, 0), key.section(QLatin1Char('/'), 1, 1)); return finish(ok, manager.resultMessage());
}
bool AuthKeysQuickConfigurationController::importKey(const QString& name, const QString& path)
{
	AuthKeysManager manager(*m_draft); auto keyName = name.isEmpty() ? manager.keyNameFromExportedKeyFile(path) : name;
	auto ok = manager.importKey(keyName, manager.detectKeyType(path), path); return finish(ok, manager.resultMessage());
}
bool AuthKeysQuickConfigurationController::exportKey(const QString& key, const QString& path)
{
	AuthKeysManager manager(*m_draft); auto ok = manager.exportKey(key.section(QLatin1Char('/'), 0, 0), key.section(QLatin1Char('/'), 1, 1), path, true); return finish(ok, manager.resultMessage());
}
bool AuthKeysQuickConfigurationController::setAccessGroup(const QString& key, const QString& group)
{
	AuthKeysManager manager(*m_draft); auto ok = manager.setAccessGroup(key, group); return finish(ok, manager.resultMessage());
}
bool AuthKeysQuickConfigurationController::test(const QString& key)
{
	const auto name = key.section(QLatin1Char('/'), 0, 0);
	if (!VeyonCore::isAuthenticationKeyNameValid(name)) return finish(false, tr("Invalid authentication key name."));
	const auto privatePath = QDir(VeyonCore::filesystem().expandPath(m_draft->privateKeyBaseDir())).filePath(name + QStringLiteral("/key"));
	const auto publicPath = QDir(VeyonCore::filesystem().expandPath(m_draft->publicKeyBaseDir())).filePath(name + QStringLiteral("/key"));
	CryptoCore::PrivateKey privateKey(privatePath); CryptoCore::PublicKey publicKey(publicPath);
	if (publicKey.isNull()) return finish(false, tr("The public key could not be read."));
	if (privateKey.isNull()) return finish(false, tr("The private key could not be read. Import the matching private key to test authentication on this computer."));
	const bool matches = privateKey.toPublicKey().toDER() == publicKey.toDER();
	return finish(matches, matches ? tr("Authentication key pair is valid and the public and private keys match.") : tr("The public and private keys do not match."));
}
