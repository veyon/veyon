// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QThread>

#include "CryptoCore.h"

class VncServerPluginInterface;

class VncServer : public QThread
{
	Q_OBJECT
public:
	using Password = CryptoCore::SecureArray;

	explicit VncServer(QObject* parent = nullptr);
	~VncServer() override;

	void prepare();

	int serverBasePort() const;
	int serverPort() const;

	Password password() const;

private:
	void run() override;

	VncServerPluginInterface* m_pluginInterface;
};
