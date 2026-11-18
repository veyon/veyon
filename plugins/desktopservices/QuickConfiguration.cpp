// SPDX-License-Identifier: GPL-2.0-or-later
#include "DesktopServicesFeaturePlugin.h"
#include "DesktopServicesConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"
#include "QuickConfigurationController.h"

QObject* DesktopServicesFeaturePlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new DesktopServicesConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->predefinedApplicationsProperty(), QStringLiteral("applications"), tr("Presets"), tr("Application presets"), Editor::Custom, {}, {}, {});
	describe(config->predefinedWebsitesProperty(), QStringLiteral("applications"), tr("Presets"), tr("Website presets"), Editor::Custom, {}, {}, {});
	return new DesktopServicesQuickConfigurationController(config, parent);
}
