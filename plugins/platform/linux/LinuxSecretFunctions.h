// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PlatformSecretFunctions.h"

#include <functional>
#include <sys/types.h>
#include <veyonconfig.h>

// clazy:excludeall=copyable-polymorphic

// SecretStore backend (03-secretstore-config.md §2): one file "<scope>__<name>" (mode 0600) per key
// in /etc/veyon/secrets.d (mode 0700), both owned by root. Secrets are read via a descriptor of the
// verified directory with O_NOFOLLOW and only from regular, single-linked files owned by root that
// nobody else can access; anything else is refused. Writes are atomic (temporary file in the same
// directory, fsync, rename, fsync of the directory).
class LinuxSecretFunctions : public PlatformSecretFunctions
{
public:
	static constexpr auto DefaultSecretDirectory = "/etc/veyon/secrets.d";

	// the default directory or, in builds with tests only, the directory in VEYON_SECRET_DIR
	LinuxSecretFunctions();
	// for tests: directory and the user owning the directory and files (root in production)
	LinuxSecretFunctions(const QString& directory, uid_t ownerUserId);

	std::optional<Veyon::Crypto::SecureBuffer> readSecret(const QString& key) override;
	bool writeSecret(const QString& key, const Veyon::Crypto::SecureBuffer& value) override;
	bool removeSecret(const QString& key) override;
	QStringList secretKeys() override;
	bool isAvailable() override;
	bool prepareStorage() override;

#if defined(VEYON_WITH_TESTS)
	// called after the temporary file has been written, before it replaces the secret
	void setBeforeRenameHook(const std::function<void()>& hook)
	{
		m_beforeRenameHook = hook;
	}
#endif

private:
	int openDirectory(bool create) const;
	void removeStaleTemporaryFiles(int directoryFd) const;
	bool isPrivateToOwner(const struct stat& status) const;

	static QByteArray fileName(const QString& key);

	QString m_directory;
	uid_t m_ownerUserId;

#if defined(VEYON_WITH_TESTS)
	std::function<void()> m_beforeRenameHook{};
#endif
};
