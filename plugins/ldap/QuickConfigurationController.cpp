// SPDX-License-Identifier: GPL-2.0-or-later
#include "QuickConfigurationController.h"
#include "LdapDirectory.h"
#include <memory>

LdapQuickConfigurationController::LdapQuickConfigurationController(LdapConfiguration* config, QObject* parent)
	: QObject(parent), m_configuration(config)
{
	config->setParent(this); setObjectName(QStringLiteral("ldap"));
	connect(static_cast<Configuration::Object*>(config->object()), &Configuration::Object::configurationChanged, this, &LdapQuickConfigurationController::changed);
}
LdapQuickConfigurationController::~LdapQuickConfigurationController()
{
	if (m_thread) { m_thread->wait(); delete m_thread; }
}
QVariantList LdapQuickConfigurationController::operations() const
{
	return {
		QVariantMap{{QStringLiteral("id"), QStringLiteral("bind")}, {QStringLiteral("label"), tr("Test connection and bind")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("baseDn")}, {QStringLiteral("label"), tr("Test base DN")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("namingContext")}, {QStringLiteral("label"), tr("Test naming context")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("userTree")}, {QStringLiteral("label"), tr("Test user tree")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("groupTree")}, {QStringLiteral("label"), tr("Test group tree")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerTree")}, {QStringLiteral("label"), tr("Test computer tree")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerGroupTree")}, {QStringLiteral("label"), tr("Test computer group tree")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("userLoginNameAttribute")}, {QStringLiteral("label"), tr("Test user login attribute")}, {QStringLiteral("argument"), tr("User login name (wildcards allowed)")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("groupMemberAttribute")}, {QStringLiteral("label"), tr("Test group member attribute")}, {QStringLiteral("argument"), tr("Group name")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerDisplayNameAttribute")}, {QStringLiteral("label"), tr("Test computer display name")}, {QStringLiteral("argument"), tr("Computer display name")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerHostNameAttribute")}, {QStringLiteral("label"), tr("Test computer hostname")}, {QStringLiteral("argument"), tr("Computer hostname")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerMacAddressAttribute")}, {QStringLiteral("label"), tr("Test computer MAC address")}, {QStringLiteral("argument"), tr("Computer distinguished name")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerLocationAttribute")}, {QStringLiteral("label"), tr("Test computer location attribute")}, {QStringLiteral("argument"), tr("Location name (wildcards allowed)")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("locationNameAttribute")}, {QStringLiteral("label"), tr("Test location name attribute")}, {QStringLiteral("argument"), tr("Location name (wildcards allowed)")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("usersFilter")}, {QStringLiteral("label"), tr("Test user filter")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("userGroupsFilter")}, {QStringLiteral("label"), tr("Test user group filter")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computersFilter")}, {QStringLiteral("label"), tr("Test computer filter")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerGroupsFilter")}, {QStringLiteral("label"), tr("Test computer group filter")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerContainersFilter")}, {QStringLiteral("label"), tr("Test computer container filter")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("userGroupsAttribute")}, {QStringLiteral("label"), tr("Test user groups attribute")}, {QStringLiteral("argument"), tr("User login name")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("userGroups")}, {QStringLiteral("label"), tr("Query all user groups")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("groupsOfUser")}, {QStringLiteral("label"), tr("Query groups of user")}, {QStringLiteral("argument"), tr("User login name")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("groupsOfComputer")}, {QStringLiteral("label"), tr("Query groups of computer")}, {QStringLiteral("argument"), tr("Computer hostname")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("computerObjectByIpAddress")}, {QStringLiteral("label"), tr("Resolve computer by IP address")}, {QStringLiteral("argument"), tr("IP address")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("locationEntries")}, {QStringLiteral("label"), tr("Query location entries")}, {QStringLiteral("argument"), tr("Location name")}},
		QVariantMap{{QStringLiteral("id"), QStringLiteral("locations")}, {QStringLiteral("label"), tr("Query all locations")}}
	};
}
void LdapQuickConfigurationController::run(const QString& operation, const QString& argument)
{
	if (busy()) return;
	m_targetKey.clear(); query(operation, argument);
}
void LdapQuickConfigurationController::browse(const QString& key, const QString& distinguishedName, bool attributes)
{
	if (busy()) return;
	m_targetKey = key; query(QStringLiteral("browse"), distinguishedName, attributes);
}
void LdapQuickConfigurationController::query(const QString& operation, const QString& argument, bool attributes)
{
	m_error.clear(); m_results.clear();
	const auto snapshot = static_cast<Configuration::Object*>(m_configuration->object())->data();
	struct Result { QStringList values; QString error; };
	auto result = std::make_shared<Result>();
	m_thread = QThread::create([snapshot, operation, argument, attributes, result]() {
		Configuration::Object draft; draft.replaceData(snapshot);
		LdapConfiguration config(&draft);
		LdapDirectory directory(config);
		auto& client = directory.client();
		if (!client.isConnected() || !client.isBound()) { result->error = tr("LDAP connection or bind failed: %1").arg(client.errorDescription()); return; }
		const auto op = operation;
		if (op == QLatin1String("bind")) result->values = {tr("Connected and bound successfully.")};
		else if (op == QLatin1String("baseDn")) result->values = client.queryBaseDn();
		else if (op == QLatin1String("namingContext")) result->values = client.queryNamingContexts(config.namingContextAttribute());
		else if (op == QLatin1String("browse"))
		{
			if (argument.isEmpty()) result->values = client.queryNamingContexts();
			else if (attributes) result->values = client.queryObjectAttributes(argument);
			else result->values = client.queryDistinguishedNames(argument, QStringLiteral("(objectclass=*)"), LdapClient::Scope::One);
		}
		else
		{
			if (op.endsWith(QLatin1String("Tree"))) { directory.disableAttributes(); directory.disableFilters(); }
			else if (op.endsWith(QLatin1String("Attribute")) && op != QLatin1String("userGroupsAttribute") && op != QLatin1String("computerLocationAttribute") && op != QLatin1String("locationNameAttribute")) directory.disableFilters();
			if (op == QLatin1String("userTree") || op == QLatin1String("userLoginNameAttribute") || op == QLatin1String("usersFilter")) result->values = directory.users(argument);
			else if (op == QLatin1String("groupTree")) result->values = directory.groups();
			else if (op == QLatin1String("computerTree") || op == QLatin1String("computersFilter")) result->values = directory.computersByHostName();
			else if (op == QLatin1String("computerGroupTree") || op == QLatin1String("computerGroupsFilter")) result->values = directory.computerGroups();
			else if (op == QLatin1String("computerDisplayNameAttribute")) result->values = directory.computersByDisplayName(argument);
			else if (op == QLatin1String("computerHostNameAttribute"))
			{
				if (config.computerHostNameAsFQDN() != argument.contains(QLatin1Char('.'))) result->error = tr("The hostname does not match the configured fully qualified hostname setting.");
				else result->values = directory.computersByHostName(argument);
			}
			else if (op == QLatin1String("computerMacAddressAttribute")) { auto mac = directory.computerMacAddress(argument); if (!mac.isEmpty()) result->values = {mac}; }
			else if (op == QLatin1String("groupMemberAttribute")) { auto groups = directory.groups(argument); if (!groups.isEmpty()) result->values = directory.groupMembers(groups.first()); }
			else if (op == QLatin1String("userGroups") || op == QLatin1String("userGroupsFilter")) result->values = directory.userGroups();
			else if (op == QLatin1String("groupsOfUser") || op == QLatin1String("userGroupsAttribute")) { auto users = directory.users(argument); if (!users.isEmpty()) result->values = directory.groupsOfUser(users.first()); }
			else if (op == QLatin1String("groupsOfComputer")) { auto computers = directory.computersByHostName(argument); if (!computers.isEmpty()) result->values = directory.groupsOfComputer(computers.first()); }
			else if (op == QLatin1String("computerObjectByIpAddress")) { auto host = directory.hostToLdapFormat(argument); if (!host.isEmpty()) result->values = directory.computersByHostName(host); else result->error = tr("Could not resolve the IP address. Check your DNS settings."); }
			else if (op == QLatin1String("locationEntries")) result->values = directory.computerLocationEntries(argument);
			else if (op == QLatin1String("locations") || op == QLatin1String("computerContainersFilter") || op == QLatin1String("computerLocationAttribute") || op == QLatin1String("locationNameAttribute")) result->values = directory.computerLocations(argument);
			else result->error = tr("Unknown LDAP operation.");
		}
		if (result->values.isEmpty() && result->error.isEmpty()) result->error = tr("No matching entries were returned. Check the configured tree, attributes and filters. %1").arg(client.errorDescription());
	});
	connect(m_thread, &QThread::finished, this, [this, result]() {
		m_results = result->values; m_error = result->error;
		m_thread->deleteLater(); m_thread = nullptr; Q_EMIT changed();
	});
	m_thread->start(); Q_EMIT changed();
}
bool LdapQuickConfigurationController::select(const QString& value)
{
	if (m_targetKey.isEmpty()) return false;
	auto property = Configuration::Property::find(m_configuration->object(), m_targetKey, QStringLiteral("LDAP"));
	if (!property || !property->isEnabled()) { m_error = tr("This setting is disabled by the current LDAP mode."); Q_EMIT changed(); return false; }
	auto selected = value;
	if (m_targetKey.endsWith(QLatin1String("Tree"))) selected = LdapClient::stripBaseDn(value, m_configuration->baseDn());
	const bool success = property->setEditorValue(selected, &m_error);
	Q_EMIT changed(); return success;
}

int LdapQuickConfigurationController::locationMode() const
{
	return m_configuration->computerLocationsByContainer() ? 1 : m_configuration->computerLocationsByAttribute() ? 2 : 0;
}
void LdapQuickConfigurationController::setLocationMode(int mode)
{
	if (mode < 0 || mode > 2) return;
	m_configuration->setComputerLocationsByContainer(mode == 1);
	m_configuration->setComputerLocationsByAttribute(mode == 2);
}
