INCLUDE(BuildPlugin)

BUILD_PLUGIN(ldap
	LdapPlugin.cpp
	LdapConfigurationPage.cpp
	LdapNetworkObjectDirectory.cpp
	MOCFILES
	LdapPlugin.h
	LdapConfigurationPage.h
	LdapNetworkObjectDirectory.h
	FORMS
	LdapConfigurationPage.ui
	RESOURCES
	ldap.qrc
)
