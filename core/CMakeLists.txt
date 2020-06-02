FILE(GLOB veyoncore_INCLUDES
						${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/include/Configuration/*.h
)
FILE(GLOB veyoncore_SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.cpp
)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/builddata.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc)
SET(core_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/core.qrc ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc)

if(NOT LibVNCClient_FOUND)
	set(libvncclient_SOURCES
		${libvncserver_DIR}/libvncclient/cursor.c
		${libvncserver_DIR}/libvncclient/listen.c
		${libvncserver_DIR}/libvncclient/rfbproto.c
		${libvncserver_DIR}/libvncclient/sockets.c
		${libvncserver_DIR}/libvncclient/tls_openssl.c
		${libvncserver_DIR}/libvncclient/vncviewer.c
		${libvncserver_DIR}/common/crypto_openssl.c
		${libvncserver_DIR}/common/turbojpeg.c)

	set_source_files_properties(${libvncclient_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unused-variable -fvisibility=default")
endif()

ADD_DEFINITIONS(-DBUILD_VEYON_CORE_LIBRARY)

ADD_LIBRARY(veyon-core SHARED ${veyoncore_SOURCES} ${veyoncore_INCLUDES} ${core_RESOURCES} ${libvncclient_SOURCES})

# find Qt's translation directory
if(NOT VEYON_BUILD_WIN32)
	include(FindQtTranslations)
	find_qt_translations()
	target_compile_definitions(veyon-core PRIVATE QT_TRANSLATIONS_DIR="${QT_TRANSLATIONS_DIR}")
endif()

set_default_target_properties(veyon-core)
target_compile_options(veyon-core PRIVATE -Wno-parentheses)

target_include_directories(veyon-core PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
	${QCA_INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES(veyon-core
	Qt5::Concurrent
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
	OpenSSL::SSL
	${VEYON_DEBUG_LIBRARIES}
	${QCA_LIBRARY}
	)

if(LibVNCClient_FOUND)
	target_link_libraries(veyon-core LibVNC::LibVNCClient)
else()
	target_include_directories(veyon-core PRIVATE
		${ZLIB_INCLUDE_DIR}
		${JPEG_INCLUDE_DIR}
		${LZO_INCLUDE_DIR}
	)
	target_include_directories(veyon-core PUBLIC
		${libvncserver_DIR}/common/
		${libvncserver_DIR}
	)
	target_link_libraries(veyon-core
		Threads::Threads
		PNG::PNG
		${ZLIB_LIBRARIES}
		${JPEG_LIBRARIES}
		${LZO_LIBRARIES}
	)
endif()

IF(VEYON_BUILD_WIN32)
	# add Windows Socket library required by libvncclient
	TARGET_LINK_LIBRARIES(veyon-core -lws2_32)
	SET_TARGET_PROPERTIES(veyon-core PROPERTIES PREFIX "")
	INSTALL(TARGETS veyon-core RUNTIME DESTINATION ${VEYON_LIB_DIR})
ELSE()
	INSTALL(TARGETS veyon-core LIBRARY DESTINATION ${VEYON_LIB_DIR})
ENDIF(VEYON_BUILD_WIN32)

COTIRE_VEYON(veyon-core)
