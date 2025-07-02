include(BuildVeyonPlugin)

add_subdirectory(kldap)
add_subdirectory(common)

build_veyon_plugin(ldap
	NAME Ldap
	SOURCES
	LdapPlugin.cpp
	LdapPlugin.h
	)

target_link_libraries(ldap PRIVATE ldap-common)
