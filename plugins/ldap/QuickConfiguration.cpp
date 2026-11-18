// SPDX-License-Identifier: GPL-2.0-or-later
#include "LdapPlugin.h"
#include "LdapConfiguration.h"
#include "QuickConfigurationPluginInterface.h"
#include "VeyonConfiguration.h"
#include "QuickConfigurationController.h"

QObject* LdapPlugin::createQuickConfiguration(Configuration::Object* draft, QObject* parent)
{
	auto config = new LdapConfiguration(draft);
	config->setParent(parent);
	using namespace QuickConfiguration;
	using Editor = Property::Editor;
	describe(config->serverHostProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Server hostname"), Editor::Automatic, {}, {}, {});
	restart(config->serverHostProperty());
	describe(config->serverPortProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Server port"), Editor::Integer, 0, 65535, QStringLiteral(""));
	restart(config->serverPortProperty());
	describe(config->connectionSecurityProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Connection security"), Editor::Automatic, {}, {}, {});
	restart(config->connectionSecurityProperty());
	describe(config->tlsVerifyModeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("TLS certificate verification"), Editor::Automatic, {}, {}, {});
	restart(config->tlsVerifyModeProperty());
	describe(config->tlsCACertificateFileProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("CA certificate file"), Editor::File, {}, {}, {});
	restart(config->tlsCACertificateFileProperty());
	describe(config->useBindCredentialsProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Use bind credentials"), Editor::Automatic, {}, {}, {});
	restart(config->useBindCredentialsProperty());
	describe(config->bindDnProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Bind distinguished name"), Editor::Automatic, {}, {}, {});
	restart(config->bindDnProperty());
	describe(config->bindPasswordProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Bind password"), Editor::Password, {}, {}, {});
	restart(config->bindPasswordProperty());
	describe(config->queryNamingContextProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Query naming context"), Editor::Automatic, {}, {}, {});
	restart(config->queryNamingContextProperty());
	describe(config->queryTimeoutProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Query timeout"), Editor::Integer, 500, 60000, QStringLiteral(" ms"));
	restart(config->queryTimeoutProperty());
	describe(config->baseDnProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Base distinguished name"), Editor::Automatic, {}, {}, {});
	restart(config->baseDnProperty());
	describe(config->namingContextAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Naming context attribute"), Editor::Automatic, {}, {}, {});
	restart(config->namingContextAttributeProperty());
	describe(config->userTreeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("User tree"), Editor::Automatic, {}, {}, {});
	restart(config->userTreeProperty());
	describe(config->groupTreeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Group tree"), Editor::Automatic, {}, {}, {});
	restart(config->groupTreeProperty());
	describe(config->computerTreeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer tree"), Editor::Automatic, {}, {}, {});
	restart(config->computerTreeProperty());
	describe(config->computerGroupTreeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer group tree"), Editor::Automatic, {}, {}, {});
	restart(config->computerGroupTreeProperty());
	describe(config->recursiveSearchOperationsProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Search subtrees recursively"), Editor::Automatic, {}, {}, {});
	restart(config->recursiveSearchOperationsProperty());
	describe(config->userLoginNameAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("User login name attribute"), Editor::Automatic, {}, {}, {});
	restart(config->userLoginNameAttributeProperty());
	describe(config->groupMemberAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Group member attribute"), Editor::Automatic, {}, {}, {});
	restart(config->groupMemberAttributeProperty());
	describe(config->computerDisplayNameAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer display name attribute"), Editor::Automatic, {}, {}, {});
	restart(config->computerDisplayNameAttributeProperty());
	describe(config->computerHostNameAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer hostname attribute"), Editor::Automatic, {}, {}, {});
	restart(config->computerHostNameAttributeProperty());
	describe(config->computerHostNameAsFQDNProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Use fully qualified hostnames"), Editor::Automatic, {}, {}, {});
	restart(config->computerHostNameAsFQDNProperty());
	describe(config->computerMacAddressAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer MAC address attribute"), Editor::Automatic, {}, {}, {});
	restart(config->computerMacAddressAttributeProperty());
	describe(config->locationNameAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Location name attribute"), Editor::Automatic, {}, {}, {});
	restart(config->locationNameAttributeProperty());
	describe(config->usersFilterProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("User filter"), Editor::Automatic, {}, {}, {});
	restart(config->usersFilterProperty());
	describe(config->userGroupsFilterProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("User group filter"), Editor::Automatic, {}, {}, {});
	restart(config->userGroupsFilterProperty());
	describe(config->computersFilterProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer filter"), Editor::Automatic, {}, {}, {});
	restart(config->computersFilterProperty());
	describe(config->queryNestedUserGroupsProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Query nested user groups"), Editor::Automatic, {}, {}, {});
	restart(config->queryNestedUserGroupsProperty());
	describe(config->queryUserGroupsFromUserAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Query groups from user attribute"), Editor::Automatic, {}, {}, {});
	restart(config->queryUserGroupsFromUserAttributeProperty());
	describe(config->userGroupsAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("User groups attribute"), Editor::Automatic, {}, {}, {});
	restart(config->userGroupsAttributeProperty());
	describe(config->mapContainerStructureToLocationsProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Map container hierarchy to locations"), Editor::Automatic, {}, {}, {});
	restart(config->mapContainerStructureToLocationsProperty());
	describe(config->identifyGroupMembersByNameAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Identify group members by name attribute"), Editor::Automatic, {}, {}, {});
	restart(config->identifyGroupMembersByNameAttributeProperty());
	describe(config->computerGroupsFilterProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer group filter"), Editor::Automatic, {}, {}, {});
	restart(config->computerGroupsFilterProperty());
	describe(config->computerContainersFilterProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer container filter"), Editor::Automatic, {}, {}, {});
	restart(config->computerContainersFilterProperty());
	describe(config->computerLocationsByContainerProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Determine locations from containers"), Editor::Automatic, {}, {}, {});
	restart(config->computerLocationsByContainerProperty());
	describe(config->computerLocationsByAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Determine locations from attribute"), Editor::Automatic, {}, {}, {});
	restart(config->computerLocationsByAttributeProperty());
	describe(config->computerLocationAttributeProperty(), QStringLiteral("ldap"), tr("LDAP / Active Directory"), tr("Computer location attribute"), Editor::Automatic, {}, {}, {});
	restart(config->computerLocationAttributeProperty());
	choices(config->connectionSecurityProperty(), {{0, tr("None")}, {1, tr("TLS")}, {2, tr("SSL")}});
	choices(config->tlsVerifyModeProperty(), {{0, tr("System default")}, {1, tr("Never verify")}, {2, tr("Custom CA certificate")}});
	depends(config->bindDnProperty(), {QStringLiteral("LDAP/UseBindCredentials")}, [config]() { return config->useBindCredentials(); });
	depends(config->bindPasswordProperty(), {QStringLiteral("LDAP/UseBindCredentials")}, [config]() { return config->useBindCredentials(); });
	depends(config->baseDnProperty(), {QStringLiteral("LDAP/QueryNamingContext")}, [config]() { return !config->queryNamingContext(); });
	depends(config->namingContextAttributeProperty(), {QStringLiteral("LDAP/QueryNamingContext")}, [config]() { return config->queryNamingContext(); });
	depends(config->userGroupsAttributeProperty(), {QStringLiteral("LDAP/QueryUserGroupsFromUserAttribute")}, [config]() { return config->queryUserGroupsFromUserAttribute(); });
	depends(config->computerLocationAttributeProperty(), {QStringLiteral("LDAP/ComputerLocationsByAttribute")}, [config]() { return config->computerLocationsByAttribute(); });
	depends(config->tlsCACertificateFileProperty(), {QStringLiteral("LDAP/TLSVerifyMode")}, [config]() { return config->tlsVerifyMode() == LdapClient::TLSVerifyCustomCert; });
	depends(config->locationNameAttributeProperty(), {QStringLiteral("LDAP/ComputerLocationsByAttribute")}, [config]() { return !config->computerLocationsByAttribute(); });
	depends(config->computerGroupTreeProperty(), {QStringLiteral("LDAP/ComputerLocationsByAttribute"), QStringLiteral("LDAP/ComputerLocationsByContainer")}, [config]() { return !config->computerLocationsByAttribute() && !config->computerLocationsByContainer(); });
	depends(config->computerContainersFilterProperty(), {QStringLiteral("LDAP/ComputerLocationsByContainer")}, [config]() { return config->computerLocationsByContainer(); });
	depends(config->mapContainerStructureToLocationsProperty(), {QStringLiteral("LDAP/ComputerLocationsByContainer")}, [config]() { return config->computerLocationsByContainer(); });
	return new LdapQuickConfigurationController(config, parent);
}
