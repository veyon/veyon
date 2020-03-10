INCLUDE(BuildVeyonPlugin)

ADD_SUBDIRECTORY(kldap)
ADD_SUBDIRECTORY(common)

build_veyon_plugin(ldap
	LdapPlugin.cpp
	LdapPlugin.h
)

TARGET_LINK_LIBRARIES(ldap ldap-common)
