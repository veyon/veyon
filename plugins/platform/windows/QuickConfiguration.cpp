// SPDX-License-Identifier: GPL-2.0-or-later
#include "WindowsPlatformPlugin.h"
#include "WindowsPlatformConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"

QObject* WindowsPlatformPlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new WindowsPlatformConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->isSoftwareSASEnabledProperty(), QStringLiteral("platform"), tr("Windows"), tr("Enable software secure attention sequence"), Editor::Automatic, {}, {}, {});
	restart(config->isSoftwareSASEnabledProperty());
	describe(config->interferingWindowsHandlingProperty(), QStringLiteral("platform"), tr("Windows"), tr("Interfering window handling"), Editor::Automatic, {}, {}, {});
	restart(config->interferingWindowsHandlingProperty());
	describe(config->disableSSPIBasedUserAuthenticationProperty(), QStringLiteral("platform"), tr("Windows"), tr("Disable SSPI authentication"), Editor::Automatic, {}, {}, {});
	restart(config->disableSSPIBasedUserAuthenticationProperty());
	describe(config->hideDesktopForScreenLockProperty(), QStringLiteral("platform"), tr("Windows"), tr("Hide desktop while screen is locked"), Editor::Automatic, {}, {}, {});
	restart(config->hideDesktopForScreenLockProperty());
	describe(config->hideTaskbarForScreenLockProperty(), QStringLiteral("platform"), tr("Windows"), tr("Hide taskbar while screen is locked"), Editor::Automatic, {}, {}, {});
	restart(config->hideTaskbarForScreenLockProperty());
	describe(config->hideStartMenuForScreenLockProperty(), QStringLiteral("platform"), tr("Windows"), tr("Hide Start menu while screen is locked"), Editor::Automatic, {}, {}, {});
	restart(config->hideStartMenuForScreenLockProperty());
	describe(config->useCustomPowerSchemeForScreenLockProperty(), QStringLiteral("platform"), tr("Windows"), tr("Use custom power scheme while screen is locked"), Editor::Automatic, {}, {}, {});
	restart(config->useCustomPowerSchemeForScreenLockProperty());
	describe(config->useInterceptionDriverProperty(), QStringLiteral("platform"), tr("Windows"), tr("Use interception driver"), Editor::Automatic, {}, {}, {});
	restart(config->useInterceptionDriverProperty());
	describe(config->disableTouchDevicesForScreenLockProperty(), QStringLiteral("platform"), tr("Windows"), tr("Disable touch devices while screen is locked"), Editor::Automatic, {}, {}, {});
	restart(config->disableTouchDevicesForScreenLockProperty());
	describe(config->disableKeyboardDevicesForScreenLockProperty(), QStringLiteral("platform"), tr("Windows"), tr("Disable keyboards while screen is locked"), Editor::Automatic, {}, {}, {});
	restart(config->disableKeyboardDevicesForScreenLockProperty());
	describe(config->disableMouseDevicesForScreenLockProperty(), QStringLiteral("platform"), tr("Windows"), tr("Disable pointing devices while screen is locked"), Editor::Automatic, {}, {}, {});
	restart(config->disableMouseDevicesForScreenLockProperty());
	describe(config->logonInputStartDelayProperty(), QStringLiteral("platform"), tr("Windows"), tr("Delay before login input"), Editor::Integer, 100, 5000, QStringLiteral(" ms"));
	restart(config->logonInputStartDelayProperty());
	describe(config->logonKeyPressIntervalProperty(), QStringLiteral("platform"), tr("Windows"), tr("Login key interval"), Editor::Integer, 5, 100, QStringLiteral(" ms"));
	restart(config->logonKeyPressIntervalProperty());
	describe(config->logonConfirmLegalNoticeProperty(), QStringLiteral("platform"), tr("Windows"), tr("Confirm legal notice during login"), Editor::Automatic, {}, {}, {});
	restart(config->logonConfirmLegalNoticeProperty());
	choices(config->interferingWindowsHandlingProperty(), {{0, tr("Do nothing")}, {1, tr("Fix window attributes")}, {2, tr("Terminate process")}, {3, tr("Close session")}});
	{ auto ui = config->logonKeyPressIntervalProperty().uiMetadata(); ui.step = 5; config->logonKeyPressIntervalProperty().setUiMetadata(ui); }
	{ auto ui = config->logonInputStartDelayProperty().uiMetadata(); ui.step = 100; config->logonInputStartDelayProperty().setUiMetadata(ui); }
	config->setObjectName(QStringLiteral("platform"));
	return config;
}
