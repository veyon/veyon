INCLUDE(BuildPlugin)

ADD_SUBDIRECTORY(kldap)
ADD_SUBDIRECTORY(common)

BUILD_PLUGIN(ldap
	LdapPlugin.cpp
	LdapPlugin.h
)

TARGET_LINK_LIBRARIES(ldap ldap-common)
