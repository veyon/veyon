/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "PluginInterface.h"
#include "VncServerPluginInterface.h"

class KWinGreeterVncServer : public QObject, public VncServerPluginInterface, public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.KWinGreeterVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	Plugin::Uid uid() const override { return QUuid(QStringLiteral("b547e4ce-27a1-40bc-8a06-6709c89980ef")); }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("KWinGreeterVncServer"); }
	QString description() const override { return tr("KWin Wayland greeter capture"); }
	QString vendor() const override { return QStringLiteral("Veyon Community"); }
	QString copyright() const override { return QStringLiteral("Veyon Community"); }
	Plugin::Flags flags() const override { return Plugin::NoFlags; }
	QStringList supportedSessionTypes() const override { return {QStringLiteral("wayland")}; }
	QWidget* configurationWidget() override { return nullptr; }
	void prepareServer() override {}
	bool runServer(int serverPort, const Password& password) override;
	int configuredServerPort() override { return -1; }
	Password configuredPassword() override { return {}; }
};
