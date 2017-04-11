FILE(GLOB veyoncore_INCLUDES
						${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/include/Configuration/*.h
)
FILE(GLOB veyoncore_SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.cpp
)

FILE(GLOB veyoncore_UI ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

IF(VEYON_BUILD_WIN32)
	SET(WIN32_AUTHSSP_SOURCES
${ultravnc_DIR}/addon/ms-logon/authSSP/authSSP.cpp
${ultravnc_DIR}/addon/ms-logon/authSSP/GenClientServerContext.cpp)
ELSE()
	SET(WIN32_AUTHSSP_SOURCES "")
ENDIF()

SET(libvncclient_SOURCES
	${libvncserver_DIR}/libvncclient/cursor.c
	${libvncserver_DIR}/libvncclient/listen.c
	${libvncserver_DIR}/libvncclient/rfbproto.c
	${libvncserver_DIR}/libvncclient/sockets.c
	${libvncserver_DIR}/libvncclient/tls_none.c
	${libvncserver_DIR}/libvncclient/vncviewer.c)


SET(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/core.qrc)
QT5_WRAP_CPP(veyoncore_MOC_out ${veyoncore_INCLUDES})
QT5_WRAP_UI(veyoncore_UIC_out ${veyoncore_UI})
QT5_ADD_RESOURCES(veyoncore_RCC_out ${QRC_FILE} ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc)
#IF(WIN32)
#	QT5_ADD_RESOURCES(veyoncore_qt_qm_out ${CMAKE_CURRENT_BINARY_DIR}/qt_qm.qrc)
#ENDIF()
#QT5_TRANSLATIONS_FOR_QRC(${QRC_FILE} ${veyoncore_SOURCES} ${veyoncore_INCLUDES})

IF(VEYON_BUILD_WIN32)
	LINK_LIBRARIES(-lws2_32 -lwtsapi32 -lnetapi32 -luserenv -ladvapi32 -linterception)
	INCLUDE_DIRECTORIES(${Qt5Gui_PRIVATE_INCLUDE_DIRS} ${ultravnc_DIR}/addon/ms-logon/authSSP)
ENDIF(VEYON_BUILD_WIN32)

ADD_DEFINITIONS(-DBUILD_VEYON_CORE_LIBRARY)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${libvncserver_DIR}/common/)
ADD_LIBRARY(veyon-core SHARED ${veyoncore_SOURCES} ${veyoncore_INCLUDES} ${veyoncore_MOC_out} ${veyoncore_UIC_out} ${veyoncore_RCC_out} ${veyoncore_qt_qm_out} ${libvncclient_SOURCES} ${WIN32_AUTHSSP_SOURCES})

IF(VEYON_BUILD_WIN32)
	SET_TARGET_PROPERTIES(veyon-core PROPERTIES PREFIX "")
	INSTALL(TARGETS veyon-core RUNTIME DESTINATION ${LIB_DIR})
ELSE()
	INSTALL(TARGETS veyon-core LIBRARY DESTINATION ${LIB_DIR})
ENDIF(VEYON_BUILD_WIN32)

TARGET_LINK_LIBRARIES(veyon-core
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
	Qt5::Xml
	${LZO_LIBRARIES}
	)

ADD_DEPENDENCIES(veyon-core GenerateTranslationFiles)

