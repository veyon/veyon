FILE(GLOB ic_INCLUDES
						${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/include/Ipc/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/include/Configuration/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/include/Ldap/*.h
)
FILE(GLOB ic_SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/Ipc/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/Ldap/*.cpp
)

SET(kldap_INCLUDES_MOCINC
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapsearch.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapmodel.h
)

SET(kldap_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapattributeproxymodel.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapstructureproxymodel.h
)

SET(kldap_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ber.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldif.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapurl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapserver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapobject.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapconnection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapoperation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapcontrol.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapsearch.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapdn.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapmodelnode_p.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapmodel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapmodel_p.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapstructureproxymodel.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src/ldapattributeproxymodel.cpp
)

FILE(GLOB ic_UI ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

IF(ITALC_BUILD_WIN32)
	SET(WIN32_AUTHSSP_SOURCES
${ULTRAVNC_DIR}/addon/ms-logon/authSSP/authSSP.cpp
${ULTRAVNC_DIR}/addon/ms-logon/authSSP/GenClientServerContext.cpp)
ELSE()
	SET(WIN32_AUTHSSP_SOURCES "")
ENDIF()

SET(LIBVNCCLIENT_SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/common/minilzo.c
${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/common/turbojpeg.c
${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/libvncclient/cursor.c
${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/libvncclient/listen.c
${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/libvncclient/rfbproto.c
${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/libvncclient/sockets.c
${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/libvncclient/tls_none.c
${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/libvncclient/vncviewer.c)


SET(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/core.qrc)
QT5_WRAP_CPP(ic_MOC_out ${ic_INCLUDES})
QT5_WRAP_CPP(kldap_MOCINC_out ${kldap_INCLUDES_MOCINC})
QT5_WRAP_CPP(kldap_MOC_out ${kldap_INCLUDES})
QT5_WRAP_UI(ic_UIC_out ${ic_UI})
QT5_ADD_RESOURCES(ic_RCC_out ${QRC_FILE})
#IF(WIN32)
#	QT5_ADD_RESOURCES(ic_qt_qm_out ${CMAKE_CURRENT_BINARY_DIR}/qt_qm.qrc)
#ENDIF()
#QT5_TRANSLATIONS_FOR_QRC(${QRC_FILE} ${ic_SOURCES} ${ic_INCLUDES})

ADD_DEFINITIONS(-DBUILD_ICA -DBUILD_LIBRARY)
IF(ITALC_BUILD_WIN32)
	LINK_LIBRARIES(-lws2_32 -lwtsapi32 -lnetapi32 -luserenv -ladvapi32 -linterception)
	SET(CMAKE_MODULE_LINKER_FLAGS -Wl,-export-all-symbols)
	INCLUDE_DIRECTORIES(${Qt5Gui_PRIVATE_INCLUDE_DIRS} ${ULTRAVNC_DIR}/addon/ms-logon/authSSP)
ENDIF(ITALC_BUILD_WIN32)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/Ldap ${CMAKE_CURRENT_SOURCE_DIR}/../service/x11/common/ ${CMAKE_CURRENT_SOURCE_DIR}/kldap/src ${CMAKE_CURRENT_BINARY_DIR}/kldap/src)
ADD_LIBRARY(italc-core SHARED ${ic_SOURCES} ${kldap_SOURCES} ${ic_INCLUDES} ${ic_MOC_out} ${kldap_MOC_out} ${ic_UIC_out} ${ic_RCC_out} ${ic_qt_qm_out} ${LIBVNCCLIENT_SOURCES} ${WIN32_AUTHSSP_SOURCES})

IF(ITALC_BUILD_WIN32)
	SET_TARGET_PROPERTIES(italc-core PROPERTIES PREFIX "")
	SET_SOURCE_FILES_PROPERTIES(src/Inject.cpp PROPERTIES COMPILE_FLAGS "-O0")
	INSTALL(TARGETS italc-core RUNTIME DESTINATION ${LIB_DIR})
ELSE()
	INSTALL(TARGETS italc-core LIBRARY DESTINATION ${LIB_DIR})
ENDIF(ITALC_BUILD_WIN32)

TARGET_LINK_LIBRARIES(italc-core
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
	Qt5::Xml
	${Ldap_LIBRARIES}
	)

ADD_DEPENDENCIES(italc-core GenerateTranslationFiles)

# create target for building included MOC files from kldap before building italc-core library
ADD_CUSTOM_TARGET(kldap_MOC ALL DEPENDS ${kldap_MOCINC_out})
ADD_DEPENDENCIES(italc-core kldap_MOC)

