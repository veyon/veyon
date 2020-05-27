include(BuildVeyonPlugin)

add_subdirectory(kldap)
add_subdirectory(common)

build_veyon_plugin(ldap
	LdapPlugin.cpp
	LdapPlugin.h
	AuthLdapConfiguration.h
	AuthLdapConfigurationDialog.cpp
	AuthLdapConfigurationDialog.h
	AuthLdapConfigurationDialog.ui
	AuthLdapCore.cpp
	AuthLdapCore.h
	AuthLdapDialog.cpp
	AuthLdapDialog.h
	AuthLdapDialog.ui
)

target_link_libraries(ldap ldap-common)
