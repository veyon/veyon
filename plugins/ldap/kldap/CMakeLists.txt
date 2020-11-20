include(BuildVeyonPlugin)

# libraries and functions for LDAP support
find_package(Ldap REQUIRED)
find_package(Sasl2 REQUIRED)

set(CMAKE_REQUIRED_INCLUDES lber.h ldap.h)
set(CMAKE_REQUIRED_LIBRARIES ${Ldap_LIBRARIES} ${Sasl2_LIBRARIES} ${OPENSSL_LIBRARIES})
check_function_exists(ldap_start_tls_s HAVE_LDAP_START_TLS_S)
check_function_exists(ldap_initialize HAVE_LDAP_INITIALIZE)
check_function_exists(ber_memfree HAVE_BER_MEMFREE)
check_function_exists(ldap_unbind_ext HAVE_LDAP_UNBIND_EXT)
check_function_exists(ldap_extended_operation HAVE_LDAP_EXTENDED_OPERATION)
check_function_exists(ldap_extended_operation_s HAVE_LDAP_EXTENDED_OPERATION_S)
CHECK_SYMBOL_EXISTS(ldap_extended_operation ldap.h HAVE_LDAP_EXTENDED_OPERATION_PROTOTYPE)
CHECK_SYMBOL_EXISTS(ldap_extended_operation_s ldap.h HAVE_LDAP_EXTENDED_OPERATION_S_PROTOTYPE)
check_include_files(ldap.h HAVE_LDAP_H)
set(LDAP_FOUND TRUE)

set(kldap_SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/kldap/src/core)
configure_file(${kldap_SOURCE_DIR}/../kldap_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kldap_config.h)

set(kldap_SOURCES
	${kldap_SOURCE_DIR}/ber.cpp
	${kldap_SOURCE_DIR}/ldif.cpp
	${kldap_SOURCE_DIR}/ldapurl.cpp
	${kldap_SOURCE_DIR}/ldapserver.cpp
	${kldap_SOURCE_DIR}/ldapobject.cpp
	${kldap_SOURCE_DIR}/ldapconnection.cpp
	${kldap_SOURCE_DIR}/ldapoperation.cpp
	${kldap_SOURCE_DIR}/ldapcontrol.cpp
	${kldap_SOURCE_DIR}/ldapdn.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/KLdapIntegration.cpp
)

add_library(kldap-light SHARED ${kldap_SOURCES})
target_compile_options(kldap-light PRIVATE ${VEYON_COMPILE_OPTIONS})
target_link_libraries(kldap-light Qt5::Core ${Ldap_LIBRARIES} ${Sasl2_LIBRARIES})
target_include_directories(kldap-light PRIVATE ${Ldap_INCLUDE_DIRS})
target_include_directories(kldap-light PUBLIC ${kldap_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
set_default_target_properties(kldap-light)
set_target_properties(kldap-light PROPERTIES LINK_FLAGS "-Wl,-no-undefined")
if(NOT WITH_CORE_ONLY)
install(TARGETS kldap-light DESTINATION ${VEYON_LIB_DIR})
endif()
cotire_veyon(kldap-light)

