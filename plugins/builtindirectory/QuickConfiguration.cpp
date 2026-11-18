// SPDX-License-Identifier: GPL-2.0-or-later
#include "BuiltinDirectoryPlugin.h"
#include "BuiltinDirectoryConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"
#include "QuickConfigurationController.h"

QObject* BuiltinDirectoryPlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new BuiltinDirectoryConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->networkObjectsProperty(), QStringLiteral("computers"), tr("Local directory"), tr("Computers and rooms"), Editor::Custom, {}, {}, {});
	return new BuiltinDirectoryQuickConfigurationController(config, parent);
}
