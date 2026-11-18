// SPDX-License-Identifier: GPL-2.0-or-later
#include "DemoFeaturePlugin.h"
#include "DemoConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* DemoFeaturePlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new DemoConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->slowDownThumbnailUpdatesProperty(), QStringLiteral("monitoring"), tr("Demonstration"), tr("Slow down thumbnail updates during demonstration"), Editor::Automatic, {}, {}, {});
	describe(config->bandwidthLimitProperty(), QStringLiteral("monitoring"), tr("Demonstration"), tr("Bandwidth limit"), Editor::Integer, 1, 1000, QStringLiteral(" MB/s"));
	describe(config->framebufferUpdateIntervalProperty(), QStringLiteral("monitoring"), tr("Demonstration"), tr("Framebuffer update interval"), Editor::Integer, 20, 500, QStringLiteral(" ms"));
	describe(config->keyFrameIntervalProperty(), QStringLiteral("monitoring"), tr("Demonstration"), tr("Key frame interval"), Editor::Integer, 1, 30, QStringLiteral(" s"));
	describe(config->memoryLimitProperty(), QStringLiteral("monitoring"), tr("Demonstration"), tr("Memory limit"), Editor::Integer, 64, 512, QStringLiteral(" MB"));
	{ auto ui = config->memoryLimitProperty().uiMetadata(); ui.step = 16; config->memoryLimitProperty().setUiMetadata(ui); }
	{ auto ui = config->framebufferUpdateIntervalProperty().uiMetadata(); ui.step = 20; config->framebufferUpdateIntervalProperty().setUiMetadata(ui); }
	config->setObjectName(QStringLiteral("monitoring"));
	return config;
}
