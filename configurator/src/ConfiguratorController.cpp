// SPDX-License-Identifier: GPL-2.0-or-later
#include "ConfiguratorController.h"
#include "AccessControlProvider.h"
#include "AccessControlRule.h"
#include "Configuration/JsonStore.h"
#include "ConfigurationManager.h"
#include "FeatureManager.h"
#include "BuiltinFeatures.h"
#include "MonitoringMode.h"
#include "NetworkObjectDirectoryManager.h"
#include "NetworkObjectDirectory.h"
#include "VncServerPluginInterface.h"
#include <QLocale>
#include "Filesystem.h"
#include "PlatformFilesystemFunctions.h"
#include "PlatformUserFunctions.h"
#include "PluginManager.h"
#include "QuickConfigurationPluginInterface.h"
#include "QuickUi.h"
#include "UserGroupsBackendManager.h"
#include "VeyonServiceControl.h"
#include <QDesktopServices>
#include <QDir>
#include <QJsonArray>
#include <QMetaEnum>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <memory>

ConfiguratorController::ConfiguratorController(QObject* parent) : QObject(parent), m_properties(nullptr)
{
	m_session.load(VeyonCore::config().data());
	for (auto plugin : VeyonCore::pluginManager().pluginObjects())
	{
		if (auto interface = qobject_cast<QuickConfigurationPluginInterface*>(plugin))
		{
			auto controller = interface->createQuickConfiguration(&m_draft, this);
			if (controller && !controller->objectName().isEmpty())
				m_plugins.insert(controller->objectName(), QVariant::fromValue(controller));
		}
	}
	using Property = Configuration::Property;
	const auto setChoices = [](Property& property, const QList<Property::Choice>& values) {
		auto ui = property.uiMetadata(); ui.editor = Property::Editor::Choice; ui.choices = values; property.setUiMetadata(ui);
	};
	QList<Property::Choice> languages{{QString(), tr("System language")}, {QStringLiteral("English - English (en_US)"), tr("English")}};
	for (const auto& file : QDir(VeyonCore::translationsDirectory()).entryList({QStringLiteral("veyon_*.qm")}))
	{
		const auto localeName = file.mid(6, file.size() - 9); const QLocale locale(localeName);
		const auto label = QStringLiteral("%1 - %2 (%3)").arg(QLocale::languageToString(locale.language()), locale.nativeLanguageName(), locale.name());
		languages.append({label, label});
	}
	setChoices(m_draft.uiLanguageProperty(), languages);
	QList<Property::Choice> directories;
	const auto availableDirectories = VeyonCore::networkObjectDirectoryManager().availableDirectories();
	for (auto it = availableDirectories.cbegin(); it != availableDirectories.cend(); ++it) directories.append({it.key(), it.value()});
	setChoices(m_draft.networkObjectDirectoryPluginProperty(), directories);
	QList<Property::Choice> backends;
	const auto availableBackends = VeyonCore::userGroupsBackendManager().availableBackends();
	for (auto it = availableBackends.cbegin(); it != availableBackends.cend(); ++it) backends.append({it.key(), it.value()});
	setChoices(m_draft.userGroupsBackendProperty(), backends);
	auto domainUi = m_draft.useDomainUserGroupsProperty().uiMetadata();
	domainUi.dependencyKeys = {m_draft.userGroupsBackendProperty().absoluteKey()};
	domainUi.enabled = [this]() { auto plugin = VeyonCore::pluginManager().pluginInterface(m_draft.userGroupsBackend()); return plugin && plugin->flags().testFlag(Plugin::ProvidesDefaultImplementation); };
	m_draft.useDomainUserGroupsProperty().setUiMetadata(domainUi);
	QList<Property::Choice> servers;
	for (auto object : VeyonCore::pluginManager().pluginObjects())
		if (qobject_cast<VncServerPluginInterface*>(object))
			if (auto plugin = qobject_cast<PluginInterface*>(object)) servers.append({plugin->uid(), plugin->description()});
	setChoices(m_draft.vncServerPluginProperty(), servers);
	QList<Property::Choice> doubleClick{{QUuid(), tr("No feature")}};
	for (const auto& feature : VeyonCore::featureManager().features())
		if (feature.testFlag(Feature::Flag::Master) && !feature.testFlag(Feature::Flag::Meta)) doubleClick.append({feature.uid(), feature.displayName()});
	setChoices(m_draft.computerDoubleClickFeatureProperty(), doubleClick);
	m_properties = new Configuration::PropertyModel(&m_draft, this);
	connect(m_properties, &Configuration::PropertyModel::valueEdited, this, [this](const QString& key) {
		// Match the exclusive session choices only for editor actions, not imported configurations.
		if (key == m_draft.activeSessionModeEnabledProperty().absoluteKey() && m_draft.activeSessionModeEnabled())
			m_draft.setMultiSessionModeEnabled(false);
		else if (key == m_draft.multiSessionModeEnabledProperty().absoluteKey() && m_draft.multiSessionModeEnabled())
			m_draft.setActiveSessionModeEnabled(false);
	});
	m_advanced = QSettings().value(QStringLiteral("Configurator/advanced"), false).toBool();
	m_properties->setAdvancedMode(m_advanced);
	connect(&m_draft, &Configuration::Object::configurationChanged, this, &ConfiguratorController::changed);
	auto timer = new QTimer(this);
	timer->setInterval(3000);
	connect(timer, &QTimer::timeout, this, &ConfiguratorController::statusChanged);
	timer->start();
}

QVariantList ConfiguratorController::pages() const
{
	return {
		QVariantMap{{QStringLiteral("id"), QStringLiteral("overview")}, {QStringLiteral("title"), tr("Overview")}, {QStringLiteral("group"), tr("Getting started")}, {QStringLiteral("icon"), QStringLiteral("layout-dashboard")}, {QStringLiteral("description"), tr("Check this computer's configuration and manage your classroom settings.")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computers")}, {QStringLiteral("title"), tr("Computers and rooms")}, {QStringLiteral("group"), tr("Connections and access")}, {QStringLiteral("icon"), QStringLiteral("monitor")}, {QStringLiteral("description"), tr("Choose and organize the computers and locations shown in Veyon Master.")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("authentication")}, {QStringLiteral("title"), tr("Sign in and keys")}, {QStringLiteral("group"), tr("Connections and access")}, {QStringLiteral("icon"), QStringLiteral("key-round")}, {QStringLiteral("description"), tr("Choose how teachers authenticate and manage authentication keys.")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("access")}, {QStringLiteral("title"), tr("Access control")}, {QStringLiteral("group"), tr("Connections and access")}, {QStringLiteral("icon"), QStringLiteral("shield")}, {QStringLiteral("description"), tr("Define which users may access this computer and test your rules.")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("usergroups")}, {QStringLiteral("title"), tr("User groups")}, {QStringLiteral("group"), tr("Connections and access")}, {QStringLiteral("icon"), QStringLiteral("users")}, {QStringLiteral("advanced"), true}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("connections")}, {QStringLiteral("title"), tr("Connections")}, {QStringLiteral("group"), tr("Connections and access")}, {QStringLiteral("icon"), QStringLiteral("network")}, {QStringLiteral("advanced"), true}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("monitoring")}, {QStringLiteral("title"), tr("Monitoring and demonstration")}, {QStringLiteral("group"), tr("Classroom features")}, {QStringLiteral("icon"), QStringLiteral("monitor")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("applications")}, {QStringLiteral("title"), tr("Applications and websites")}, {QStringLiteral("group"), tr("Classroom features")}, {QStringLiteral("icon"), QStringLiteral("app-window")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("filetransfer")}, {QStringLiteral("title"), tr("File transfer")}, {QStringLiteral("group"), tr("Classroom features")}, {QStringLiteral("icon"), QStringLiteral("folder-up")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("service")}, {QStringLiteral("title"), tr("Veyon service")}, {QStringLiteral("group"), tr("System")}, {QStringLiteral("icon"), QStringLiteral("server-cog")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("ldap")}, {QStringLiteral("title"), tr("LDAP / Active Directory")}, {QStringLiteral("group"), tr("System")}, {QStringLiteral("icon"), QStringLiteral("network")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("logs")}, {QStringLiteral("title"), tr("Logs")}, {QStringLiteral("group"), tr("System")}, {QStringLiteral("icon"), QStringLiteral("scroll-text")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("interface")}, {QStringLiteral("title"), tr("User interface")}, {QStringLiteral("group"), tr("System")}, {QStringLiteral("icon"), QStringLiteral("palette")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("webapi")}, {QStringLiteral("title"), tr("Web API")}, {QStringLiteral("group"), tr("System")}, {QStringLiteral("icon"), QStringLiteral("globe")}, {QStringLiteral("advanced"), true}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("platform")}, {QStringLiteral("title"), tr("Platform settings")}, {QStringLiteral("group"), tr("System")}, {QStringLiteral("icon"), QStringLiteral("settings")}, {QStringLiteral("advanced"), true}}
	};
}

void ConfiguratorController::setAdvanced(bool value)
{
	if (m_advanced == value) return;
	m_advanced = value;
	QSettings().setValue(QStringLiteral("Configurator/advanced"), value);
	m_properties->setAdvancedMode(value);
	Q_EMIT advancedChanged();
}

bool ConfiguratorController::fail(const QString& message)
{
	m_error = message; Q_EMIT changed(); return false;
}

bool ConfiguratorController::apply(bool forceSave)
{
	if (!dirty() && !forceSave) return true;
	const bool restart = m_session.requiresServiceRestart();
	ConfigurationManager manager(m_draft);
	if (!manager.saveConfiguration()) return fail(manager.errorString());
	VeyonCore::config().replaceData(m_draft.data());
	m_session.saved(restart);
	if (!manager.applyConfiguration())
		return fail(tr("The settings were saved, but could not be fully applied. Retry Apply to finish: %1").arg(manager.errorString()));
	VeyonCore::userGroupsBackendManager().reloadConfiguration();
	if (auto backend = VeyonCore::userGroupsBackendManager().configuredBackend()) backend->reloadConfiguration();
	const bool recommendRestart = m_session.restartPending();
	m_session.applied(); m_error.clear(); Q_EMIT changed();
	if (recommendRestart && serviceRunning()) Q_EMIT restartRecommended();
	return true;
}

void ConfiguratorController::revert()
{
	m_session.revert(); if (!m_session.applyPending()) m_error.clear();
	m_properties->refresh(); Q_EMIT changed();
}

void ConfiguratorController::factoryReset()
{
	m_session.factoryReset(); if (!m_session.applyPending()) m_error.clear(); m_properties->refresh(); Q_EMIT changed();
}

bool ConfiguratorController::importConfiguration(const QString& path)
{
	if (path.isEmpty()) return false;
	QString error;
	if (!m_session.importConfiguration(path, &error)) return fail(error);
	m_properties->refresh(); if (!m_session.applyPending()) m_error.clear(); Q_EMIT changed(); return true;
}

bool ConfiguratorController::exportConfiguration(const QString& path)
{
	if (path.isEmpty()) return false;
	QString error;
	const auto file = path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive) ? path : path + QStringLiteral(".json");
	if (!m_session.exportConfiguration(file, &error)) return fail(error);
	return true;
}

QString ConfiguratorController::chooseFile(bool save, const QString& filter)
{
	auto result = QuickUi::execDialog(QStringLiteral("file"), {{QStringLiteral("title"), save ? tr("Export") : tr("Import")}, {QStringLiteral("save"), save}, {QStringLiteral("filter"), filter}});
	return result.value(QStringLiteral("accepted")).toBool() ? result.value(QStringLiteral("path")).toString() : QString();
}
QString ConfiguratorController::chooseDirectory()
{
	auto result = QuickUi::execDialog(QStringLiteral("folder"), {{QStringLiteral("title"), tr("Choose directory")}});
	return result.value(QStringLiteral("accepted")).toBool() ? result.value(QStringLiteral("path")).toString() : QString();
}
bool ConfiguratorController::serviceRunning() const { return VeyonServiceControl().isServiceRunning(); }
bool ConfiguratorController::service(const QString& operation)
{
	VeyonServiceControl service;
	if (operation == QLatin1String("start")) service.startService();
	else if (operation == QLatin1String("stop")) service.stopService();
	else service.restartService();
	bool ok = service.isServiceRunning() == (operation != QLatin1String("stop"));
	Q_EMIT statusChanged();
	return ok || fail(tr("The service operation failed. Check your permissions and the service log."));
}
void ConfiguratorController::openLogDirectory()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(VeyonCore::filesystem().expandPath(m_draft.logFileDirectory())));
}
bool ConfiguratorController::clearLogs()
{
	const bool running = serviceRunning();
	if (running && !service(QStringLiteral("stop"))) return false;
	bool success = true;
	for (const auto& path : {VeyonCore::filesystem().expandPath(m_draft.logFileDirectory()), VeyonCore::platform().filesystemFunctions().globalTempPath()})
	{
		QDir directory(path);
		for (const auto& file : directory.entryList({QStringLiteral("Veyon*.log")}, QDir::Files))
			if (!file.startsWith(QLatin1String("VeyonConfigurator"))) success &= directory.remove(file);
	}
	if (running) success &= service(QStringLiteral("start"));
	return success || fail(tr("Could not remove all log files or restart the service."));
}
bool ConfiguratorController::testAuthentication(const QString& username, const QString& password)
{
	if (m_draft.authenticationMethod() == VeyonCore::AuthenticationMethod::LogonAuthentication)
	{
		if (!VeyonCore::platform().userFunctions().authenticate(username, CryptoCore::SecureArray(password.toUtf8())))
			return fail(tr("Authentication failed."));
	}
	else if (m_draft.authenticationMethod() == VeyonCore::AuthenticationMethod::KeyFileAuthentication)
	{
		const QDir directory(VeyonCore::filesystem().expandPath(m_draft.privateKeyBaseDir()));
		const auto selectedKey = qEnvironmentVariable("VEYON_AUTH_KEY_NAME");
		const auto names = selectedKey.isEmpty() ? directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name) : QStringList{selectedKey};
		bool readablePrivateKey = false;
		for (const auto& name : names)
		{
			if (!VeyonCore::isAuthenticationKeyNameValid(name)) continue;
			const CryptoCore::PrivateKey key(directory.filePath(name + QStringLiteral("/key")));
			if (!key.isNull() && key.isPrivate()) { readablePrivateKey = true; break; }
		}
		if (!readablePrivateKey)
			return fail(selectedKey.isEmpty() ? tr("No readable private authentication key was found in %1.").arg(directory.path()) :
				tr("The selected private authentication key “%1” is invalid or could not be read.").arg(selectedKey));
	}
	else return fail(tr("Select a supported authentication method."));
	m_error.clear(); Q_EMIT changed(); return true;
}

QVariantList ConfiguratorController::rules() const
{
	QVariantList result;
	for (const auto& item : m_draft.accessControlRules())
	{
		AccessControlRule rule(item);
		QVariantList conditions;
		for (const auto& condition : conditionTypes())
		{
			const auto c = static_cast<AccessControlRule::Condition>(condition.toMap().value(QStringLiteral("value")).toInt());
			conditions.append(QVariantMap{{QStringLiteral("type"), int(c)}, {QStringLiteral("enabled"), rule.isConditionEnabled(c)}, {QStringLiteral("subject"), int(rule.subject(c))}, {QStringLiteral("argument"), rule.argument(c)}});
		}
		result.append(QVariantMap{{QStringLiteral("name"), rule.name()}, {QStringLiteral("description"), rule.description()}, {QStringLiteral("action"), int(rule.action())}, {QStringLiteral("ignore"), rule.areConditionsIgnored()}, {QStringLiteral("invert"), rule.areConditionsInverted()}, {QStringLiteral("conditions"), conditions}});
	}
	return result;
}
QVariantList ConfiguratorController::conditionTypes() const
{
	return {QVariantMap{{QStringLiteral("value"), 1}, {QStringLiteral("label"), tr("Member of user group")}}, QVariantMap{{QStringLiteral("value"), 2}, {QStringLiteral("label"), tr("Groups in common")}}, QVariantMap{{QStringLiteral("value"), 3}, {QStringLiteral("label"), tr("Located at")}}, QVariantMap{{QStringLiteral("value"), 4}, {QStringLiteral("label"), tr("Same location")}}, QVariantMap{{QStringLiteral("value"), 5}, {QStringLiteral("label"), tr("Access from local host")}}, QVariantMap{{QStringLiteral("value"), 6}, {QStringLiteral("label"), tr("Access from local user")}}, QVariantMap{{QStringLiteral("value"), 7}, {QStringLiteral("label"), tr("Already connected user")}}, QVariantMap{{QStringLiteral("value"), 8}, {QStringLiteral("label"), tr("No user logged on")}}, QVariantMap{{QStringLiteral("value"), 13}, {QStringLiteral("label"), tr("Computer already being accessed")}}};
}
QVariantMap ConfiguratorController::newRule() const
{
	QVariantList conditions;
	for (const auto& item : conditionTypes()) conditions.append(QVariantMap{{QStringLiteral("type"), item.toMap().value(QStringLiteral("value"))}, {QStringLiteral("enabled"), false}, {QStringLiteral("subject"), item.toMap().value(QStringLiteral("value")).toInt() == 3 ? 2 : 1}, {QStringLiteral("argument"), QStringLiteral("")}});
	return {{QStringLiteral("name"), tr("New rule")}, {QStringLiteral("description"), QStringLiteral("")}, {QStringLiteral("action"), 1}, {QStringLiteral("ignore"), false}, {QStringLiteral("invert"), false}, {QStringLiteral("conditions"), conditions}};
}
bool ConfiguratorController::saveRule(int index, const QVariantMap& data)
{
	if (data.value(QStringLiteral("name")).toString().trimmed().isEmpty()) return fail(tr("Enter a name for the rule."));
	AccessControlRule rule(index >= 0 && index < m_draft.accessControlRules().size() ? m_draft.accessControlRules().at(index) : QJsonValue(QJsonObject()));
	rule.setName(data.value(QStringLiteral("name")).toString()); rule.setDescription(data.value(QStringLiteral("description")).toString());
	rule.setAction(static_cast<AccessControlRule::Action>(data.value(QStringLiteral("action")).toInt()));
	rule.setConditionsIgnored(data.value(QStringLiteral("ignore")).toBool()); rule.setConditionsInverted(data.value(QStringLiteral("invert")).toBool());
	for (const auto& item : data.value(QStringLiteral("conditions")).toList())
	{
		auto values = item.toMap(); auto condition = static_cast<AccessControlRule::Condition>(values.value(QStringLiteral("type")).toInt());
		rule.setConditionEnabled(condition, values.value(QStringLiteral("enabled")).toBool());
		rule.setSubject(condition, static_cast<AccessControlRule::Subject>(values.value(QStringLiteral("subject")).toInt()));
		rule.setArgument(condition, values.value(QStringLiteral("argument")).toString());
	}
	auto array = m_draft.accessControlRules();
	if (index < 0) array.append(rule.toJson()); else if (index < array.size()) array[index] = rule.toJson(); else return false;
	m_draft.setAccessControlRules(array); return true;
}
void ConfiguratorController::removeRule(int index)
{
	auto array = m_draft.accessControlRules(); if (index < 0 || index >= array.size()) return;
	array.removeAt(index); m_draft.setAccessControlRules(array);
}
void ConfiguratorController::moveRule(int index, int delta)
{
	auto array = m_draft.accessControlRules(); if (index < 0 || index >= array.size() || index + delta < 0 || index + delta >= array.size()) return;
	auto item = array.takeAt(index); array.insert(index + delta, item); m_draft.setAccessControlRules(array);
}
QString ConfiguratorController::testRules(const QVariantMap& scenario)
{
	if (m_session.applyPending()) return tr("Finish applying the saved settings before testing access.");
	if (m_session.hasChangesInGroups({QStringLiteral("LDAP"), QStringLiteral("BuiltinDirectory"), QStringLiteral("UserGroups"), QStringLiteral("NetworkObjectDirectory")}))
		return tr("Apply directory and user-group backend changes before testing membership or location conditions. Rule edits can be tested without applying them.");
	std::unique_ptr<NetworkObjectDirectory> directory(VeyonCore::networkObjectDirectoryManager().createDirectory(m_draft.networkObjectDirectoryPlugin(), nullptr));
	AccessControlProvider provider(m_draft, nullptr, directory.get());
	auto rule = provider.processAccessControlRules(scenario.value(QStringLiteral("accessingUser")).toString(), scenario.value(QStringLiteral("accessingComputer")).toString(), scenario.value(QStringLiteral("localUser")).toString(), scenario.value(QStringLiteral("localComputer")).toString(), scenario.value(QStringLiteral("connectedUsers")).toString().split(QLatin1Char(','), Qt::SkipEmptyParts));
	if (!rule) return tr("No rule matches. Access is denied.");
	return tr("Matching rule: %1. Result: %2").arg(rule->name(), rule->action() == AccessControlRule::Action::Allow ? tr("Allowed") : rule->action() == AccessControlRule::Action::AskForPermission ? tr("Ask for permission") : tr("Denied"));
}
QVariantList ConfiguratorController::features() const
{
	QVariantList result;
	for (const auto& feature : VeyonCore::featureManager().features())
		if (feature.testFlag(Feature::Flag::Master) && !feature.testFlag(Feature::Flag::Meta) && feature != VeyonCore::builtinFeatures().monitoringMode().feature())
		result.append(QVariantMap{{QStringLiteral("uid"), feature.uid().toString()}, {QStringLiteral("name"), feature.displayName()}, {QStringLiteral("enabled"), !m_draft.disabledFeatures().contains(feature.uid().toString())}});
	return result;
}
void ConfiguratorController::setFeatureEnabled(const QString& uid, bool enabled)
{
	auto disabled = m_draft.disabledFeatures();
	if (enabled) disabled.removeAll(uid); else if (!disabled.contains(uid)) disabled.append(uid);
	m_draft.setDisabledFeatures(disabled);
}

int ConfiguratorController::accessMode() const
{
	return m_draft.isAccessRestrictedToUserGroups() ? 1 : m_draft.isAccessControlRulesProcessingEnabled() ? 2 : 0;
}
void ConfiguratorController::setAccessMode(int mode)
{
	if (mode < 0 || mode > 2) return;
	m_draft.setAccessRestrictedToUserGroups(mode == 1);
	m_draft.setAccessControlRulesProcessingEnabled(mode == 2);
}
QStringList ConfiguratorController::availableGroups()
{
	if (m_session.applyPending() || m_session.hasChangesInGroups({QStringLiteral("LDAP"), QStringLiteral("UserGroups")}))
	{
		fail(tr("Apply changes to the user-group backend before testing group membership."));
		return {};
	}
	return AccessControlProvider(m_draft).userGroups();
}
QString ConfiguratorController::testUserGroupAccess(const QString& username)
{
	if (m_session.applyPending()) return tr("Finish applying the saved settings before testing access.");
	if (username.trimmed().isEmpty()) return tr("Enter a user login name.");
	if (m_session.hasChangesInGroups({QStringLiteral("LDAP"), QStringLiteral("UserGroups")}))
		return tr("Apply changes to the user-group backend before testing group membership.");
	return AccessControlProvider(m_draft).processAuthorizedGroups(username) ? tr("The user is allowed access by the selected groups.") : tr("The user is not allowed access by the selected groups.");
}
QStringList ConfiguratorController::ruleArguments(int condition)
{
	if (condition == 1) return availableGroups();
	if (condition != 3) return {};
	if (m_session.applyPending() || m_session.hasChangesInGroups({QStringLiteral("LDAP"), QStringLiteral("BuiltinDirectory"), QStringLiteral("NetworkObjectDirectory")}))
	{
		fail(tr("Apply directory and user-group backend changes before testing membership or location conditions. Rule edits can be tested without applying them."));
		return {};
	}
	std::unique_ptr<NetworkObjectDirectory> directory(VeyonCore::networkObjectDirectoryManager().createDirectory(m_draft.networkObjectDirectoryPlugin(), nullptr));
	return AccessControlProvider(m_draft, nullptr, directory.get()).locations();
}
