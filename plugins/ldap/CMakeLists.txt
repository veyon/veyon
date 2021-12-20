include(BuildVeyonPlugin)

add_subdirectory(kldap)
add_subdirectory(common)

build_veyon_plugin(ldap
	LdapPlugin.cpp
	LdapPlugin.h
	)

target_link_libraries(ldap PRIVATE ldap-common)
