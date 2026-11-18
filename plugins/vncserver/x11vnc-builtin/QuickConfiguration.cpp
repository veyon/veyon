// SPDX-License-Identifier: GPL-2.0-or-later
#include "BuiltinX11VncServer.h"
#include "X11VncConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* BuiltinX11VncServer::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new X11VncConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->isXDamageDisabledProperty(), QStringLiteral("connections"), tr("X11 VNC server"), tr("Disable XDamage extension"), Editor::Automatic, {}, {}, {});
	restart(config->isXDamageDisabledProperty());
	describe(config->extraArgumentsProperty(), QStringLiteral("connections"), tr("X11 VNC server"), tr("Extra x11vnc arguments"), Editor::Automatic, {}, {}, {});
	restart(config->extraArgumentsProperty());
	for (auto property : draft->findChildren<Property*>())
	{
		if (property->uiMetadata().groupId != tr("X11 VNC server")) continue;
		auto ui = property->uiMetadata();
		ui.dependencyKeys.append(QStringLiteral("VncServer/Plugin"));
		const auto pluginUid = uid();
		ui.visible = [draft, pluginUid]() { return static_cast<VeyonConfiguration*>(draft)->vncServerPlugin() == pluginUid; };
		property->setUiMetadata(ui);
	}
	config->setObjectName(QStringLiteral("connections"));
	return config;
}
