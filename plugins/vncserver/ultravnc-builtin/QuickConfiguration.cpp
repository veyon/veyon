// SPDX-License-Identifier: GPL-2.0-or-later
#include "BuiltinUltraVncServer.h"
#include "UltraVncConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* BuiltinUltraVncServer::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new UltraVncConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->ultraVncCaptureLayeredWindowsProperty(), QStringLiteral("connections"), tr("UltraVNC server"), tr("Capture layered windows"), Editor::Automatic, {}, {}, {});
	restart(config->ultraVncCaptureLayeredWindowsProperty());
	describe(config->ultraVncMultiMonitorSupportEnabledProperty(), QStringLiteral("connections"), tr("UltraVNC server"), tr("Enable multiple monitors"), Editor::Automatic, {}, {}, {});
	restart(config->ultraVncMultiMonitorSupportEnabledProperty());
	describe(config->ultraVncPollFullScreenProperty(), QStringLiteral("connections"), tr("UltraVNC server"), tr("Poll full screen"), Editor::Automatic, {}, {}, {});
	restart(config->ultraVncPollFullScreenProperty());
	describe(config->ultraVncLowAccuracyProperty(), QStringLiteral("connections"), tr("UltraVNC server"), tr("Use low accuracy"), Editor::Automatic, {}, {}, {});
	restart(config->ultraVncLowAccuracyProperty());
	describe(config->ultraVncDeskDupEngineEnabledProperty(), QStringLiteral("connections"), tr("UltraVNC server"), tr("Use desktop duplication engine"), Editor::Automatic, {}, {}, {});
	restart(config->ultraVncDeskDupEngineEnabledProperty());
	describe(config->ultraVncMaxCpuProperty(), QStringLiteral("connections"), tr("UltraVNC server"), tr("Maximum CPU usage"), Editor::Integer, 10, 100, QStringLiteral(" %"));
	restart(config->ultraVncMaxCpuProperty());
	for (auto property : draft->findChildren<Property*>())
	{
		if (property->uiMetadata().groupId != tr("UltraVNC server")) continue;
		auto ui = property->uiMetadata();
		ui.dependencyKeys.append(QStringLiteral("VncServer/Plugin"));
		const auto pluginUid = uid();
		ui.visible = [draft, pluginUid]() { return static_cast<VeyonConfiguration*>(draft)->vncServerPlugin() == pluginUid; };
		property->setUiMetadata(ui);
	}
	config->setObjectName(QStringLiteral("connections"));
	return config;
}
