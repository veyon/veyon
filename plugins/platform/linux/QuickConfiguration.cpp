// SPDX-License-Identifier: GPL-2.0-or-later
#include "LinuxPlatformPlugin.h"
#include "LinuxPlatformConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* LinuxPlatformPlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new LinuxPlatformConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->pamServiceNameProperty(), QStringLiteral("platform"), tr("Linux"), tr("PAM service name"), Editor::Automatic, {}, {}, {});
	restart(config->pamServiceNameProperty());
	describe(config->minimumUserSessionLifetimeProperty(), QStringLiteral("platform"), tr("Linux"), tr("Minimum session lifetime"), Editor::Integer, 1, 60, QStringLiteral(" s"));
	restart(config->minimumUserSessionLifetimeProperty());
	describe(config->userLoginKeySequenceProperty(), QStringLiteral("platform"), tr("Linux"), tr("Login key sequence"), Editor::Automatic, {}, {}, {});
	restart(config->userLoginKeySequenceProperty());
	for (auto property : draft->findChildren<Property*>())
		if (property->absoluteKey() == QStringLiteral("Linux/UserLoginSession"))
		{
			describe(*property, QStringLiteral("platform"), tr("Linux"), tr("LightDM login session"), Editor::Text);
			auto ui = property->uiMetadata();
			ui.toolTip = tr("Session to start for remote login through LightDM, for example plasma. Leave empty to use the default session.");
			property->setUiMetadata(ui);
			restart(*property);
		}
	describe(config->userLoginInputStartDelayProperty(), QStringLiteral("platform"), tr("Linux"), tr("Delay before login input"), Editor::Integer, 100, 5000, QStringLiteral(" ms"));
	restart(config->userLoginInputStartDelayProperty());
	describe(config->userLoginTextInputKeyPressIntervalProperty(), QStringLiteral("platform"), tr("Linux"), tr("Text input key interval"), Editor::Integer, 5, 100, QStringLiteral(" ms"));
	restart(config->userLoginTextInputKeyPressIntervalProperty());
	describe(config->userLoginControlKeyPressIntervalProperty(), QStringLiteral("platform"), tr("Linux"), tr("Control key interval"), Editor::Integer, 5, 100, QStringLiteral(" ms"));
	restart(config->userLoginControlKeyPressIntervalProperty());
	{ auto ui = config->userLoginTextInputKeyPressIntervalProperty().uiMetadata(); ui.step = 5; config->userLoginTextInputKeyPressIntervalProperty().setUiMetadata(ui); }
	{ auto ui = config->userLoginInputStartDelayProperty().uiMetadata(); ui.step = 100; config->userLoginInputStartDelayProperty().setUiMetadata(ui); }
	{ auto ui = config->userLoginControlKeyPressIntervalProperty().uiMetadata(); ui.step = 5; config->userLoginControlKeyPressIntervalProperty().setUiMetadata(ui); }
	config->setObjectName(QStringLiteral("platform"));
	return config;
}
