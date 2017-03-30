INCLUDE(BuildPlugin)

BUILD_PLUGIN(ldap
	LdapPlugin.cpp
	LdapNetworkObjectDirectory.cpp
	MOCFILES
	LdapPlugin.h
	LdapNetworkObjectDirectory.h
)
