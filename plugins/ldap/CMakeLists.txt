include(BuildPlugin)

add_subdirectory(kldap)
add_subdirectory(common)

build_plugin(ldap
	LdapPlugin.cpp
	LdapPlugin.h
)

target_link_libraries(ldap ldap-common)
