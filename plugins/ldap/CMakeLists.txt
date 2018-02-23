INCLUDE(BuildPlugin)

# libraries and functions for LDAP support
FIND_PACKAGE(Ldap REQUIRED)
SET(CMAKE_REQUIRED_INCLUDES lber.h ldap.h)
SET(CMAKE_REQUIRED_LIBRARIES ${Ldap_LIBRARIES})
CHECK_FUNCTION_EXISTS(ldap_start_tls_s HAVE_LDAP_START_TLS_S)
CHECK_FUNCTION_EXISTS(ldap_initialize HAVE_LDAP_INITIALIZE)
CHECK_FUNCTION_EXISTS(ber_memfree HAVE_BER_MEMFREE)
CHECK_FUNCTION_EXISTS(ldap_unbind_ext HAVE_LDAP_UNBIND_EXT)
CHECK_FUNCTION_EXISTS(ldap_extended_operation HAVE_LDAP_EXTENDED_OPERATION)
CHECK_FUNCTION_EXISTS(ldap_extended_operation_s HAVE_LDAP_EXTENDED_OPERATION_S)
CHECK_SYMBOL_EXISTS(ldap_extended_operation ldap.h HAVE_LDAP_EXTENDED_OPERATION_PROTOTYPE)
CHECK_SYMBOL_EXISTS(ldap_extended_operation_s ldap.h HAVE_LDAP_EXTENDED_OPERATION_S_PROTOTYPE)
CHECK_INCLUDE_FILES(ldap.h HAVE_LDAP_H)
SET(LDAP_FOUND TRUE)

SET(kldap_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/kldap/src)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/3rdparty/kldap/src/kldap_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kldap_config.h)

SET(kldap_SOURCES
  ${kldap_SOURCE_DIR}/ber.cpp
  ${kldap_SOURCE_DIR}/ldif.cpp
  ${kldap_SOURCE_DIR}/ldapurl.cpp
  ${kldap_SOURCE_DIR}/ldapserver.cpp
  ${kldap_SOURCE_DIR}/ldapobject.cpp
  ${kldap_SOURCE_DIR}/ldapconnection.cpp
  ${kldap_SOURCE_DIR}/ldapoperation.cpp
  ${kldap_SOURCE_DIR}/ldapcontrol.cpp
  ${kldap_SOURCE_DIR}/ldapdn.cpp
)

BUILD_PLUGIN(ldap
	LdapPlugin.cpp
	LdapConfiguration.cpp
	LdapConfigurationPage.cpp
	LdapDirectory.cpp
	LdapNetworkObjectDirectory.cpp
	KLdapIntegration.cpp
	${kldap_SOURCES}
	MOCFILES
	LdapPlugin.h
	LdapConfiguration.h
	LdapConfigurationPage.h
	LdapDirectory.h
	LdapNetworkObjectDirectory.h
	FORMS
	LdapConfigurationPage.ui
	RESOURCES
	ldap.qrc
	COTIRE
)

INCLUDE_DIRECTORIES(${Ldap_INCLUDE_DIRS} ${kldap_SOURCE_DIR})

TARGET_LINK_LIBRARIES(ldap ${Ldap_LIBRARIES})

