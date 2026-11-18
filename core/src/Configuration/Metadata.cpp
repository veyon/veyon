// SPDX-License-Identifier: GPL-2.0-or-later
#include "Configuration/Metadata.h"
#include "Configuration/Property.h"
#include "VeyonConfiguration.h"

namespace Configuration
{

void Metadata::initialize(VeyonConfiguration& c)
{
	using Editor = Property::Editor;
	using Effect = Property::ApplyEffect;
	QString page;
	QString group;
	Property::ApplyEffects effects;
	int order = 0;
	const auto section = [&](const QString& newPage, const QString& newGroup,
		Property::ApplyEffects newEffects = {}) {
		page = newPage;
		group = newGroup;
		effects = newEffects;
	};
	const auto field = [&](Property& property, const QString& label, Editor editor = Editor::Automatic,
		QVariant minimum = {}, QVariant maximum = {}, const QString& suffix = {}) {
		Property::UiMetadata metadata;
		metadata.pageId = page;
		metadata.groupId = group;
		metadata.label = label;
		metadata.editor = editor;
		metadata.minimum = minimum;
		metadata.maximum = maximum;
		metadata.suffix = suffix;
		metadata.effects = effects;
		metadata.order = order++;
		property.setUiMetadata(metadata);
	};
	const auto choices = [](Property& property, const QList<Property::Choice>& values) {
		auto metadata = property.uiMetadata();
		metadata.choices = values;
		metadata.editor = Editor::Choice;
		property.setUiMetadata(metadata);
	};
	const auto dependency = [](Property& property, const Property& source, std::function<bool()> enabled) {
		auto metadata = property.uiMetadata();
		metadata.dependencyKeys = {source.absoluteKey()};
		metadata.enabled = std::move(enabled);
		property.setUiMetadata(metadata);
	};
	const auto help = [](Property& property, const QString& text) {
		auto metadata = property.uiMetadata();
		metadata.toolTip = text;
		property.setUiMetadata(metadata);
	};

	section(QStringLiteral("internal"), tr("Internal configuration"));
	field(c.applicationVersionProperty(), tr("Configuration version"));
	field(c.pluginVersionsProperty(), tr("Plugin versions"), Editor::Custom);
	field(c.installationIDProperty(), tr("Installation identifier"));
	field(c.computerStatePollingIntervalProperty(), tr("Computer state polling interval"), Editor::Integer, -1, 2147483647, tr("ms"));

	section(QStringLiteral("connections"), tr("VNC connection timing"));
	field(c.useCustomVncConnectionSettingsProperty(), tr("Use custom VNC connection settings"));
	field(c.vncConnectionThreadTerminationTimeoutProperty(), tr("Thread termination timeout"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionConnectTimeoutProperty(), tr("Connection timeout"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionReadTimeoutProperty(), tr("Read timeout"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionRetryIntervalProperty(), tr("Connection retry interval"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionMessageWaitTimeoutProperty(), tr("Message wait timeout"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionFastFramebufferUpdateIntervalProperty(), tr("Fast framebuffer update interval"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionInitialFramebufferUpdateTimeoutProperty(), tr("Initial framebuffer update timeout"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionFramebufferUpdateTimeoutProperty(), tr("Framebuffer update timeout"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionSocketKeepaliveIdleTimeProperty(), tr("Socket keepalive idle time"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionSocketKeepaliveIntervalProperty(), tr("Socket keepalive interval"), Editor::Integer, 0, 2147483647, tr("ms"));
	field(c.vncConnectionSocketKeepaliveCountProperty(), tr("Socket keepalive count"), Editor::Integer, 0, 2147483647);

	section(QStringLiteral("interface"), tr("Appearance"), Effect::ApplicationRestart);
	field(c.uiLanguageProperty(), tr("Language"), Editor::Choice);
	field(c.uiStyleProperty(), tr("Style"));
	choices(c.uiStyleProperty(), {{0, tr("Fusion")}, {1, tr("Native")}});
	field(c.uiColorSchemeProperty(), tr("Color scheme"));
	choices(c.uiColorSchemeProperty(), {{0, tr("System")}, {1, tr("Light")}, {2, tr("Dark")}});
	help(c.uiLanguageProperty(), tr("Restart Veyon applications after changing the language."));

	section(QStringLiteral("service"), tr("Notifications"), Effect::ServiceRestart);
	field(c.isTrayIconHiddenProperty(), tr("Hide tray icon"));
	field(c.failedAuthenticationNotificationsEnabledProperty(), tr("Notify about failed authentication"));
	field(c.remoteConnectionNotificationsEnabledProperty(), tr("Notify about remote connections"));
	section(QStringLiteral("service"), tr("Sessions"), Effect::ServiceRestart);
	field(c.activeSessionModeEnabledProperty(), tr("Only serve the active session"));
	field(c.multiSessionModeEnabledProperty(), tr("Enable multi-session mode"));
	field(c.maximumSessionCountProperty(), tr("Maximum number of sessions"), Editor::Integer, 1, 1000);
	dependency(c.maximumSessionCountProperty(), c.multiSessionModeEnabledProperty(), [&c]() { return c.multiSessionModeEnabled(); });
	field(c.serverConnectionLimitProperty(), tr("Maximum number of server connections"), Editor::Integer, 1, 10000);
	field(c.autostartServiceProperty(), tr("Start service automatically"));
	{
		auto metadata = c.autostartServiceProperty().uiMetadata();
		metadata.effects = Effect::Platform;
		c.autostartServiceProperty().setUiMetadata(metadata);
	}
	field(c.clipboardSynchronizationDisabledProperty(), tr("Disable clipboard synchronization"));
	section(QStringLiteral("service"), tr("Session metadata"), Effect::ServiceRestart);
	field(c.sessionMetaDataContentProperty(), tr("Session metadata source"));
	choices(c.sessionMetaDataContentProperty(), {{0, tr("None")}, {1, tr("Environment variable")}, {2, tr("Registry key")}});
	field(c.sessionMetaDataEnvironmentVariableProperty(), tr("Environment variable"));
	dependency(c.sessionMetaDataEnvironmentVariableProperty(), c.sessionMetaDataContentProperty(), [&c]() {
		return c.sessionMetaDataContent() == PlatformSessionFunctions::SessionMetaDataContent::EnvironmentVariable;
	});
	field(c.sessionMetaDataRegistryKeyProperty(), tr("Registry key"));
	dependency(c.sessionMetaDataRegistryKeyProperty(), c.sessionMetaDataContentProperty(), [&c]() {
		return c.sessionMetaDataContent() == PlatformSessionFunctions::SessionMetaDataContent::RegistryKey;
	});
#ifndef Q_OS_WIN
	{
		auto metadata = c.sessionMetaDataRegistryKeyProperty().uiMetadata();
		metadata.visible = []() { return false; };
		c.sessionMetaDataRegistryKeyProperty().setUiMetadata(metadata);
	}
#endif

	section(QStringLiteral("computers"), tr("Computer directory"), Effect::ServiceRestart);
	field(c.networkObjectDirectoryPluginProperty(), tr("Computer and location directory"), Editor::Choice);
	field(c.networkObjectDirectoryUpdateIntervalProperty(), tr("Directory update interval"), Editor::Integer,
		NetworkObjectDirectory::MinimumUpdateInterval, NetworkObjectDirectory::MaximumUpdateInterval, tr("s"));
	section(QStringLiteral("usergroups"), tr("User groups"), Effect::ServiceRestart);
	field(c.userGroupsBackendProperty(), tr("User groups backend"), Editor::Choice);
	field(c.useDomainUserGroupsProperty(), tr("Use domain user groups"));
	section(QStringLiteral("monitoring"), tr("Features"));
	field(c.disabledFeaturesProperty(), tr("Disabled features"), Editor::Custom);

	section(QStringLiteral("logs"), tr("Logging"), Effect::ServiceRestart);
	field(c.logLevelProperty(), tr("Log level"));
	choices(c.logLevelProperty(), {{0, tr("Nothing")}, {1, tr("Critical")}, {2, tr("Error")}, {3, tr("Warning")}, {4, tr("Information")}, {5, tr("Debug")}});
	field(c.logFileDirectoryProperty(), tr("Log file directory"), Editor::Directory);
	field(c.logFileSizeLimitEnabledProperty(), tr("Limit log file size"));
	field(c.logFileSizeLimitProperty(), tr("Maximum log file size"), Editor::Integer, 1, 999, tr("MB"));
	dependency(c.logFileSizeLimitProperty(), c.logFileSizeLimitEnabledProperty(), [&c]() { return c.logFileSizeLimitEnabled(); });
	field(c.logFileRotationEnabledProperty(), tr("Rotate log files"));
	dependency(c.logFileRotationEnabledProperty(), c.logFileSizeLimitEnabledProperty(), [&c]() { return c.logFileSizeLimitEnabled(); });
	field(c.logFileRotationCountProperty(), tr("Number of rotated log files"), Editor::Integer, 1, 10);
	dependency(c.logFileRotationCountProperty(), c.logFileRotationEnabledProperty(), [&c]() { return c.logFileRotationEnabled(); });
	field(c.logToStdErrProperty(), tr("Log to standard error"));
	field(c.logToSystemProperty(), tr("Log to the system journal"));

	section(QStringLiteral("connections"), tr("Network ports"), Effect::ServiceRestart);
	field(c.veyonServerPortProperty(), tr("Veyon server port"), Editor::Integer, 1024, 65535);
	field(c.vncServerPortProperty(), tr("VNC server port"), Editor::Integer, 1024, 65535);
	field(c.featureWorkerManagerPortProperty(), tr("Feature worker manager port"), Editor::Integer, 1024, 65535);
	field(c.demoServerPortProperty(), tr("Demonstration server port"), Editor::Integer, 1024, 65535);
	field(c.isFirewallExceptionEnabledProperty(), tr("Enable firewall exception"));
	{
		auto metadata = c.isFirewallExceptionEnabledProperty().uiMetadata();
		metadata.effects = Effect::Platform;
		c.isFirewallExceptionEnabledProperty().setUiMetadata(metadata);
	}
	field(c.localConnectOnlyProperty(), tr("Allow connections from the local computer only"));
	section(QStringLiteral("connections"), tr("VNC server"), Effect::ServiceRestart);
	field(c.vncServerPluginProperty(), tr("VNC server backend"), Editor::Choice);

	section(QStringLiteral("monitoring"), tr("Directories"));
	field(c.configurationTemplatesDirectoryProperty(), tr("Configuration templates directory"), Editor::Directory);
	field(c.userConfigurationDirectoryProperty(), tr("User configuration directory"), Editor::Directory);
	field(c.screenshotDirectoryProperty(), tr("Screenshot directory"), Editor::Directory);
	section(QStringLiteral("monitoring"), tr("Monitoring"));
	field(c.computerMonitoringUpdateIntervalProperty(), tr("Update interval"), Editor::Integer, 250, 10000, tr("ms"));
	{
		auto metadata = c.computerMonitoringUpdateIntervalProperty().uiMetadata();
		metadata.step = 250;
		c.computerMonitoringUpdateIntervalProperty().setUiMetadata(metadata);
	}
	field(c.computerMonitoringImageQualityProperty(), tr("Thumbnail image quality"));
	const QList<Property::Choice> qualityChoices{{0, tr("Highest")}, {1, tr("High")}, {2, tr("Medium")}, {3, tr("Low")}, {4, tr("Lowest")}};
	choices(c.computerMonitoringImageQualityProperty(), qualityChoices);
	field(c.computerMonitoringThumbnailSpacingProperty(), tr("Thumbnail spacing"), Editor::Integer, 2, 50, tr("px"));
	field(c.computerDisplayRoleContentProperty(), tr("Thumbnail caption"));
	choices(c.computerDisplayRoleContentProperty(), {{0, tr("User and computer name")}, {1, tr("User name")}, {2, tr("Computer name")}});
	field(c.computerUidRoleContentProperty(), tr("Computer identifier"));
	choices(c.computerUidRoleContentProperty(), {{0, tr("Network object identifier")}, {1, tr("Session metadata hash")}});
	field(c.computerMonitoringGridSizingModeProperty(), tr("Grid sizing"));
	choices(c.computerMonitoringGridSizingModeProperty(), {{0, tr("Full-length captions")}, {1, tr("Maximized thumbnails")}});
	field(c.computerMonitoringVisibilityModeProperty(), tr("Thumbnail visibility"));
	choices(c.computerMonitoringVisibilityModeProperty(), {{0, tr("Normal")}, {1, tr("Blurred")}, {2, tr("Hidden")}});
	field(c.computerMonitoringSortOrderProperty(), tr("Sort computers by"));
	choices(c.computerMonitoringSortOrderProperty(), {{0, tr("Computer and user name")}, {1, tr("User name")}, {2, tr("Computer name")}, {3, tr("Last part of user name")}});
	field(c.computerMonitoringAspectRatioProperty(), tr("Thumbnail aspect ratio"));
	choices(c.computerMonitoringAspectRatioProperty(), {{0, tr("Automatic")}, {1, QStringLiteral("16:9")}, {2, QStringLiteral("16:10")}, {3, QStringLiteral("3:2")}, {4, QStringLiteral("4:3")}});
	field(c.computerMonitoringBackgroundColorProperty(), tr("Thumbnail background color"));
	field(c.computerMonitoringTextColorProperty(), tr("Thumbnail text color"));
	field(c.autoAdjustMonitoringIconSizeProperty(), tr("Automatically adjust thumbnail size"));
	field(c.computerNameSourceProperty(), tr("Computer name source"));
	choices(c.computerNameSourceProperty(), {{0, tr("Directory")}, {1, tr("Host address")}, {2, tr("Session client address")}, {3, tr("Session client name")}, {4, tr("Session host name")}, {5, tr("Session metadata")}, {6, tr("User full name")}, {7, tr("User login name")}});

	section(QStringLiteral("monitoring"), tr("Locations"));
	field(c.autoSelectCurrentLocationProperty(), tr("Automatically select the current location"));
	field(c.showCurrentLocationOnlyProperty(), tr("Show the current location only"));
	field(c.allowAddingHiddenLocationsProperty(), tr("Allow adding hidden locations"));
	dependency(c.allowAddingHiddenLocationsProperty(), c.showCurrentLocationOnlyProperty(), [&c]() { return c.showCurrentLocationOnly(); });
	field(c.expandLocationsProperty(), tr("Expand locations"));
	field(c.hideLocalComputerProperty(), tr("Hide this computer"));
	field(c.hideOwnSessionProperty(), tr("Hide the current session"));
	field(c.hideEmptyLocationsProperty(), tr("Hide empty locations"));
	field(c.hideComputerFilterProperty(), tr("Hide computer filter"));
	section(QStringLiteral("monitoring"), tr("Startup and actions"));
	field(c.accessControlForMasterEnabledProperty(), tr("Check access control when starting Veyon Master"));
	field(c.computerDoubleClickFeatureProperty(), tr("Double-click action"), Editor::Choice);
	field(c.enforceSelectedModeForClientsProperty(), tr("Enforce the selected mode for clients"));
	field(c.autoOpenComputerSelectPanelProperty(), tr("Open the computer selection panel on startup"));
	field(c.confirmUnsafeActionsProperty(), tr("Confirm potentially unsafe actions"));
	field(c.showFeatureWindowsOnSameScreenProperty(), tr("Show feature windows on the same screen"));
	section(QStringLiteral("monitoring"), tr("Guest users"), Effect::ServiceRestart);
	field(c.guestUserPropertyProperty(), tr("Identify guest users by"));
	choices(c.guestUserPropertyProperty(), {{0, tr("Disabled")}, {1, tr("Login name")}, {2, tr("Full name")}});
	field(c.guestUserValueProperty(), tr("Guest user name"));
	dependency(c.guestUserValueProperty(), c.guestUserPropertyProperty(), [&c]() { return c.guestUserProperty() != PlatformUserFunctions::UserProperty::None; });
	field(c.guestUserIdentityExtensionTypeProperty(), tr("Guest identity extension"));
	choices(c.guestUserIdentityExtensionTypeProperty(), {{0, tr("None")}, {1, tr("Prefix")}, {2, tr("Suffix")}});
	field(c.guestUserIdentityExtensionValueProperty(), tr("Guest identity extension text"));
	dependency(c.guestUserIdentityExtensionValueProperty(), c.guestUserIdentityExtensionTypeProperty(), [&c]() {
		return c.guestUserIdentityExtensionType() != MonitoringMode::GuestUserIdentityExtensionType::None;
	});
	section(QStringLiteral("connections"), tr("Remote access"));
	field(c.remoteAccessImageQualityProperty(), tr("Remote access image quality"));
	choices(c.remoteAccessImageQualityProperty(), qualityChoices);

	section(QStringLiteral("authentication"), tr("Authentication"), Effect::ServiceRestart);
	field(c.authenticationMethodProperty(), tr("Authentication method"));
	choices(c.authenticationMethodProperty(), {{0, tr("Logon authentication")}, {1, tr("Key file authentication")}});
	field(c.privateKeyBaseDirProperty(), tr("Private key directory"), Editor::Directory);
	field(c.publicKeyBaseDirProperty(), tr("Public key directory"), Editor::Directory);
	help(c.privateKeyBaseDirProperty(), tr("Directory containing private authentication keys. Path variables are supported."));
	help(c.publicKeyBaseDirProperty(), tr("Directory containing public authentication keys. Path variables are supported."));
	section(QStringLiteral("access"), tr("Access policy"), Effect::ServiceRestart);
	field(c.isAccessRestrictedToUserGroupsProperty(), tr("Restrict access to authorized user groups"));
	field(c.isAccessControlRulesProcessingEnabledProperty(), tr("Process access control rules"));
	field(c.authorizedUserGroupsProperty(), tr("Authorized user groups"), Editor::Custom);
	field(c.accessControlRulesProperty(), tr("Access control rules"), Editor::Custom);

	section(QStringLiteral("internal"), tr("Legacy settings"));
	field(c.legacyAccessControlUserGroupsBackendProperty(), tr("Legacy user groups backend"));
	field(c.legacyDomainGroupsForAccessControlEnabledProperty(), tr("Legacy domain groups"));
	field(c.legacyAutoAdjustGridSizeProperty(), tr("Legacy automatic grid size"));
	field(c.legacyOpenComputerManagementAtStartProperty(), tr("Legacy startup computer management"));
	field(c.legacyAutoSwitchToCurrentRoomProperty(), tr("Legacy current room selection"));
	field(c.legacyOnlyCurrentRoomVisibleProperty(), tr("Legacy current room visibility"));
	field(c.legacyManualRoomAdditionAllowedProperty(), tr("Legacy manual room addition"));
	field(c.legacyEmptyRoomsHiddenProperty(), tr("Legacy empty room visibility"));
	field(c.legacyConfirmDangerousActionsProperty(), tr("Legacy action confirmation"));
	field(c.legacyIsSoftwareSASEnabledProperty(), tr("Legacy software secure attention sequence"));
	field(c.legacyLocalComputerHiddenProperty(), tr("Legacy local computer visibility"));
	field(c.legacyComputerFilterHiddenProperty(), tr("Legacy computer filter visibility"));
	field(c.legacyPrimaryServicePortProperty(), tr("Legacy service port"));
	for (auto property : {static_cast<Property*>(&c.legacyAccessControlUserGroupsBackendProperty()),
		static_cast<Property*>(&c.legacyDomainGroupsForAccessControlEnabledProperty())})
	{
		auto metadata = property->uiMetadata();
		metadata.visible = []() { return false; };
		property->setUiMetadata(metadata);
	}
}

}
