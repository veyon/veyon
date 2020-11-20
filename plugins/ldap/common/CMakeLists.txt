SET(ldap_common_SOURCES
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
	LdapDirectory.cpp
	LdapDirectory.h
	LdapNetworkObjectDirectory.cpp
	LdapNetworkObjectDirectory.h
	ldap.qrc
	)

ADD_LIBRARY(ldap-common SHARED ${ldap_common_SOURCES})
TARGET_INCLUDE_DIRECTORIES(ldap-common PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
TARGET_INCLUDE_DIRECTORIES(ldap-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(ldap-common kldap-light veyon-core)
set_default_target_properties(ldap-common)
SET_TARGET_PROPERTIES(ldap-common PROPERTIES LINK_FLAGS "-Wl,-no-undefined")
if(NOT WITH_CORE_ONLY)
INSTALL(TARGETS ldap-common DESTINATION ${VEYON_LIB_DIR})
endif()
COTIRE_VEYON(ldap-common)
