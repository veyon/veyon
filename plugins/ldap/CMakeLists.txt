include(BuildVeyonPlugin)

add_subdirectory(kldap)

build_veyon_plugin(ldap
	NAME Ldap
	SOURCES
	LdapPlugin.cpp
	LdapPlugin.h
	AuthLdapConfiguration.h
	AuthLdapConfigurationWidget.cpp
	AuthLdapConfigurationWidget.h
	AuthLdapConfigurationWidget.ui
	AuthLdapCore.cpp
	AuthLdapCore.h
	AuthLdapDialog.cpp
	AuthLdapDialog.h
	AuthLdapDialog.ui
	LdapBrowseDialog.cpp
	LdapBrowseDialog.ui
	LdapBrowseDialog.h
	LdapBrowseModel.cpp
	LdapBrowseModel.h
	LdapClient.cpp
	LdapClient.h
	LdapConfiguration.cpp
	LdapConfiguration.h
	LdapConfigurationPage.h
	LdapConfigurationPage.cpp
	LdapConfigurationPage.ui
	LdapConfigurationTest.h
	LdapConfigurationTest.cpp
	LdapDirectory.cpp
	LdapDirectory.h
	LdapNetworkObjectDirectory.cpp
	LdapNetworkObjectDirectory.h
	LdapNetworkObjectDirectoryConfigurationPage.cpp
	LdapNetworkObjectDirectoryConfigurationPage.h
	LdapNetworkObjectDirectoryConfigurationPage.ui
	ldap.qrc)

target_include_directories(ldap PRIVATE ${Ldap_INCLUDE_DIRS})
target_include_directories(ldap PUBLIC )
target_link_libraries(ldap PRIVATE kldap-light)
