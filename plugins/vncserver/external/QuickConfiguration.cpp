// SPDX-License-Identifier: GPL-2.0-or-later
#include "ExternalVncServer.h"
#include "ExternalVncServerConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* ExternalVncServer::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new ExternalVncServerConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->serverPortProperty(), QStringLiteral("connections"), tr("External VNC server"), tr("External VNC server port"), Editor::Integer, 1024, 65535, QStringLiteral(""));
	restart(config->serverPortProperty());
	describe(config->passwordProperty(), QStringLiteral("connections"), tr("External VNC server"), tr("External VNC password"), Editor::Password, {}, {}, {});
	restart(config->passwordProperty());
	for (auto property : draft->findChildren<Property*>())
	{
		if (property->uiMetadata().groupId != tr("External VNC server")) continue;
		auto ui = property->uiMetadata();
		ui.dependencyKeys.append(QStringLiteral("VncServer/Plugin"));
		const auto pluginUid = uid();
		ui.visible = [draft, pluginUid]() { return static_cast<VeyonConfiguration*>(draft)->vncServerPlugin() == pluginUid; };
		property->setUiMetadata(ui);
	}
	config->setObjectName(QStringLiteral("connections"));
	return config;
}
