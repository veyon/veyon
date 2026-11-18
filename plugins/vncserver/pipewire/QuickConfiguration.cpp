// SPDX-License-Identifier: GPL-2.0-or-later
#include "PipeWireVncServer.h"
#include "PipeWireVncConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* PipeWireVncServer::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new PipeWireVncConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->persistRestoreTokenProperty(), QStringLiteral("connections"), tr("PipeWire VNC server"), tr("Remember screen sharing permission"), Editor::Automatic, {}, {}, {});
	restart(config->persistRestoreTokenProperty());
	for (auto property : draft->findChildren<Property*>())
	{
		if (property->uiMetadata().groupId != tr("PipeWire VNC server")) continue;
		auto ui = property->uiMetadata();
		ui.dependencyKeys.append(QStringLiteral("VncServer/Plugin"));
		const auto pluginUid = uid();
		ui.visible = [draft, pluginUid]() { return static_cast<VeyonConfiguration*>(draft)->vncServerPlugin() == pluginUid; };
		property->setUiMetadata(ui);
	}
	config->setObjectName(QStringLiteral("connections"));
	return config;
}
