INCLUDE(BuildPlugin)

SET(kldap_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/kldap/src)
SET(kldap_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/__/__/3rdparty/kldap/src)

SET(kldap_INCLUDES_MOCINC
  ${kldap_SOURCE_DIR}/ldapsearch.h
  ${kldap_SOURCE_DIR}/ldapmodel.h
)

SET(kldap_INCLUDES
  ${kldap_SOURCE_DIR}/ldapattributeproxymodel.h
  ${kldap_SOURCE_DIR}/ldapstructureproxymodel.h
)

SET(kldap_SOURCES
  ${kldap_SOURCE_DIR}/ber.cpp
  ${kldap_SOURCE_DIR}/ldif.cpp
  ${kldap_SOURCE_DIR}/ldapurl.cpp
  ${kldap_SOURCE_DIR}/ldapserver.cpp
  ${kldap_SOURCE_DIR}/ldapobject.cpp
  ${kldap_SOURCE_DIR}/ldapconnection.cpp
  ${kldap_SOURCE_DIR}/ldapoperation.cpp
  ${kldap_SOURCE_DIR}/ldapcontrol.cpp
  ${kldap_SOURCE_DIR}/ldapsearch.cpp
  ${kldap_SOURCE_DIR}/ldapdn.cpp
  ${kldap_SOURCE_DIR}/ldapmodelnode_p.cpp
  ${kldap_SOURCE_DIR}/ldapmodel.cpp
  ${kldap_SOURCE_DIR}/ldapmodel_p.cpp
  ${kldap_SOURCE_DIR}/ldapstructureproxymodel.cpp
  ${kldap_SOURCE_DIR}/ldapattributeproxymodel.cpp
)

QT5_WRAP_CPP(kldap_MOCINC_out ${kldap_INCLUDES_MOCINC})
QT5_WRAP_CPP(kldap_MOC_out ${kldap_INCLUDES})

BUILD_PLUGIN(ldap
	LdapPlugin.cpp
	LdapConfiguration.cpp
	LdapConfigurationPage.cpp
	LdapDirectory.cpp
	LdapNetworkObjectDirectory.cpp
	KLdapIntegration.cpp
	${kldap_SOURCES}
	${kldap_MOC_out}
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
)

INCLUDE_DIRECTORIES(${Ldap_INCLUDE_DIRS} ${kldap_SOURCE_DIR} ${kldap_BINARY_DIR})

TARGET_LINK_LIBRARIES(ldap ${Ldap_LIBRARIES})

IF(SASL2_FOUND)
TARGET_LINK_LIBRARIES(ldap ${Sasl2_LIBRARIES})
ENDIF()

# create target for building included MOC files from kldap before building LDAP plugin
ADD_CUSTOM_TARGET(kldap_MOC ALL DEPENDS ${kldap_MOCINC_out})
ADD_DEPENDENCIES(ldap kldap_MOC)
