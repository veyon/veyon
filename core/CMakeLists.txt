FILE(GLOB veyoncore_INCLUDES
						${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/include/Configuration/*.h
)
FILE(GLOB veyoncore_SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.cpp
)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/builddata.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc)
SET(core_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/core.qrc ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc)

# find Qt's translation directory
IF(NOT VEYON_BUILD_WIN32)
	GET_TARGET_PROPERTY(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
	EXECUTE_PROCESS(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
						OUTPUT_STRIP_TRAILING_WHITESPACE
						OUTPUT_VARIABLE QT_TRANSLATIONS_DIR)
	ADD_DEFINITIONS(-D'QT_TRANSLATIONS_DIR="${QT_TRANSLATIONS_DIR}"')
ENDIF()


SET(libvncclient_SOURCES
	${libvncserver_DIR}/libvncclient/cursor.c
	${libvncserver_DIR}/libvncclient/listen.c
	${libvncserver_DIR}/libvncclient/rfbproto.c
	${libvncserver_DIR}/libvncclient/sockets.c
	${libvncserver_DIR}/libvncclient/tls_openssl.c
	${libvncserver_DIR}/libvncclient/vncviewer.c
	${libvncserver_DIR}/common/d3des.c
	${libvncserver_DIR}/common/turbojpeg.c)

SET_SOURCE_FILES_PROPERTIES(${libvncclient_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unused-variable -Wno-incompatible-pointer-types")

ADD_LIBRARY(veyon-core SHARED ${veyoncore_SOURCES} ${veyoncore_INCLUDES} ${core_RESOURCES} ${libvncclient_SOURCES})

TARGET_INCLUDE_DIRECTORIES(veyon-core PUBLIC
	${CMAKE_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${libvncserver_DIR}/common/
	${libvncserver_DIR}
	${QCA_INCLUDE_DIR}
)

TARGET_INCLUDE_DIRECTORIES(veyon-core PRIVATE SYSTEM ${ZLIB_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${LZO_INCLUDE_DIR})

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/core)


TARGET_LINK_LIBRARIES(veyon-core
	Qt5::Concurrent
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
	Threads::Threads
	${VEYON_DEBUG_LIBRARIES}
	${ZLIB_LIBRARIES}
	${JPEG_LIBRARIES}
	${PNG_LIBRARIES}
	${LZO_LIBRARIES}
	${QCA_LIBRARY}
	${OPENSSL_LIBRARIES}
	)

IF(VEYON_BUILD_WIN32)
	# add Windows Socket library required by libvncclient
	TARGET_LINK_LIBRARIES(veyon-core -lws2_32)
	SET_TARGET_PROPERTIES(veyon-core PROPERTIES PREFIX "")
	INSTALL(TARGETS veyon-core RUNTIME DESTINATION ${VEYON_LIB_DIR})
ELSE()
	INSTALL(TARGETS veyon-core LIBRARY DESTINATION ${VEYON_LIB_DIR})
ENDIF(VEYON_BUILD_WIN32)

COTIRE_VEYON(veyon-core)
