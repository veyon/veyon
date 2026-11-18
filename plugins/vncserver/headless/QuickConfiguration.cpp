// SPDX-License-Identifier: GPL-2.0-or-later
#include "HeadlessVncServer.h"
#include "HeadlessVncConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* HeadlessVncServer::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new HeadlessVncConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->backgroundColorProperty(), QStringLiteral("connections"), tr("Headless VNC server"), tr("Background color"), Editor::Color, {}, {}, {});
	restart(config->backgroundColorProperty());
	for (auto property : draft->findChildren<Property*>())
	{
		if (property->uiMetadata().groupId != tr("Headless VNC server")) continue;
		auto ui = property->uiMetadata();
		ui.dependencyKeys.append(QStringLiteral("VncServer/Plugin"));
		const auto pluginUid = uid();
		ui.visible = [draft, pluginUid]() { return static_cast<VeyonConfiguration*>(draft)->vncServerPlugin() == pluginUid; };
		property->setUiMetadata(ui);
	}
	config->setObjectName(QStringLiteral("connections"));
	return config;
}
