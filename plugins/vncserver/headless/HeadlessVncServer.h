// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "HeadlessVncConfiguration.h"
#include "PluginInterface.h"
#include "VncServerPluginInterface.h"

struct HeadlessVncScreen;

class HeadlessVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.HeadlessVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	explicit HeadlessVncServer(QObject* parent = nullptr);

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{QStringLiteral("f626f759-7691-45c0-bd4a-37171d98d219")};
	}

	QVersionNumber version() const override
	{
		return QVersionNumber(1, 0);
	}

	QString name() const override
	{
		return QStringLiteral("HeadlessVncServer");
	}

	QString description() const override
	{
		return tr("Headless VNC server");
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	Plugin::Flags flags() const override
	{
		return Plugin::NoFlags;
	}

	virtual QStringList supportedSessionTypes() const override
	{
		return {};
	}

	QWidget* configurationWidget() override
	{
		return nullptr;
	}

	void prepareServer() override;

	bool runServer(int serverPort, const Password& password) override;

	int configuredServerPort() override
	{
		return -1;
	}

	Password configuredPassword() override
	{
		return {};
	}

private:
	static constexpr auto DefaultFramebufferWidth = 640;
	static constexpr auto DefaultFramebufferHeight = 480;
	static constexpr auto DefaultSleepTime = 25;

	bool initScreen(HeadlessVncScreen* screen);
	bool initVncServer(int serverPort, const VncServerPluginInterface::Password& password,
					   HeadlessVncScreen* screen);

	bool handleScreenChanges(HeadlessVncScreen* screen);

	static void rfbLogDebug(const char* format, ...);
	static void rfbLogNone(const char* format, ...);

	HeadlessVncConfiguration m_configuration;
};
