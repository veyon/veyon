INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

SET(kldap_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/kldap/src)

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

IF(SASL2_FOUND)
TARGET_LINK_LIBRARIES(ldap ${Sasl2_LIBRARIES})
ENDIF()
