file(GLOB veyoncore_SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.h
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/builddata.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc)
set(core_RESOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/resources/core.qrc
	${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc
)

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

add_library(veyon-core SHARED ${veyoncore_SOURCES} ${core_RESOURCES} ${libvncclient_SOURCES})

set_default_target_properties(veyon-core)

target_compile_options(veyon-core PRIVATE ${VEYON_COMPILE_OPTIONS} -Wno-parentheses)

target_include_directories(veyon-core PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_BINARY_DIR}/src
	${QCA_INCLUDE_DIR}
)

target_link_libraries(veyon-core
	Qt5::Concurrent
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
	Qt5::QuickControls2
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

# find Qt's translation directory
if(NOT VEYON_BUILD_WIN32)
	include(FindQtTranslations)
	find_qt_translations()
	target_compile_definitions(veyon-core PRIVATE QT_TRANSLATIONS_DIR="${QT_TRANSLATIONS_DIR}")
endif()

if(VEYON_BUILD_WIN32)
	# add Windows Socket library required by libvncclient
	target_link_libraries(veyon-core -lws2_32)
	set_target_properties(veyon-core PROPERTIES PREFIX "")
	if(NOT WITH_CORE_ONLY)
		install(TARGETS veyon-core RUNTIME DESTINATION ${VEYON_LIB_DIR})
	endif()
else()
	if(NOT WITH_CORE_ONLY)
		install(TARGETS veyon-core LIBRARY DESTINATION ${VEYON_LIB_DIR})
	endif()
endif()

if(VEYON_BUILD_ANDROID)
	target_link_libraries(veyon-core Qt5::AndroidExtras)
endif()

cotire_veyon(veyon-core)
