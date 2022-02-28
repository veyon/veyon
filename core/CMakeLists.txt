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

find_package(LibVNCClient 0.9.13)

if(LibVNCClient_FOUND)
	include(CheckCSourceCompiles)
	set(CMAKE_REQUIRED_LIBRARIES LibVNC::LibVNCClient)
	check_c_source_compiles("
		#include <rfb/rfbclient.h>

		int main()
		{
		rfbClient* client = rfbGetClient( 8, 3, 4 );
		client->ReadFromSocket = NULL;
		client->WriteToSocket = NULL;
		return 0;
		}
		"
		LIBVNCCLIENT_SUPPORTS_CUSTOM_SOCKET_IO)
	if(NOT LIBVNCCLIENT_SUPPORTS_CUSTOM_SOCKET_IO)
		message(WARNING "Outdated development version of LibVNCClient found")
		unset(LibVNCClient_FOUND)
	endif()
	unset(CMAKE_REQUIRED_LIBRARIES)
endif()

if(LibVNCClient_FOUND)
	set_property(GLOBAL PROPERTY HAVE_LIBVNCCLIENT TRUE)
else()
	message(WARNING "Performing internal build of LibVNCClient which requires additional development packages")
	include(LibVNCServerIntegration)
	set(libvncclient_SOURCES
		${libvncserver_DIR}/libvncclient/cursor.c
		${libvncserver_DIR}/libvncclient/listen.c
		${libvncserver_DIR}/libvncclient/rfbproto.c
		${libvncserver_DIR}/libvncclient/sockets.c
		${libvncserver_DIR}/libvncclient/tls_openssl.c
		${libvncserver_DIR}/libvncclient/vncviewer.c
		${libvncserver_DIR}/common/crypto_openssl.c
		${libvncserver_DIR}/common/turbojpeg.c)

	set_source_files_properties(${libvncclient_SOURCES} PROPERTIES
		COMPILE_FLAGS "-Wno-unused-function -Wno-unused-variable -fvisibility=default"
		SKIP_PRECOMPILE_HEADERS TRUE)
endif()

add_library(veyon-core SHARED ${veyoncore_SOURCES} ${core_RESOURCES} ${libvncclient_SOURCES})

target_compile_definitions(veyon-core PRIVATE XK_KOREAN)

set_default_target_properties(veyon-core)

target_compile_options(veyon-core PRIVATE -Wno-parentheses)

target_include_directories(veyon-core PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_BINARY_DIR}/src
	${QCA_INCLUDE_DIR}
	)

if(WITH_QT6)
	target_link_libraries(veyon-core PUBLIC
		Qt6::Concurrent
		Qt6::Gui
		Qt6::Network
		Qt6::Widgets
		Qt6::QuickControls2)
	if(WITH_TESTS)
		target_link_libraries(veyon-core PUBLIC Qt6::Test)
	endif()
else()
	target_link_libraries(veyon-core PUBLIC
		Qt5::Concurrent
		Qt5::Gui
		Qt5::Network
		Qt5::Widgets
		Qt5::QuickControls2)
	if(WITH_TESTS)
		target_link_libraries(veyon-core PUBLIC Qt5::Test)
	endif()
endif()

target_link_libraries(veyon-core PUBLIC
	OpenSSL::SSL
	${QCA_LIBRARY})

if(LibVNCClient_FOUND)
	target_link_libraries(veyon-core PRIVATE LibVNC::LibVNCClient)
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
	target_link_libraries(veyon-core PRIVATE
		Threads::Threads
		PNG::PNG
		${ZLIB_LIBRARIES}
		${JPEG_LIBRARIES}
		${LZO_LIBRARIES}
		)
endif()

if(VEYON_BUILD_WINDOWS)
	# add Windows Socket library required by libvncclient
	target_link_libraries(veyon-core PRIVATE -lws2_32)
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
	target_link_libraries(veyon-core PRIVATE Qt5::AndroidExtras)
endif()

if(WITH_PCH)
	set(PCH ${CMAKE_CURRENT_SOURCE_DIR}/src/PrecompiledHeader.h)
	target_precompile_headers(veyon-core PRIVATE ${PCH})

	add_pch_target(veyon-application-pch ${PCH})
	set_target_properties(veyon-application-pch PROPERTIES COMPILE_FLAGS "${CMAKE_COMPILE_FLAG_PIE}")

	add_pch_target(veyon-library-pch ${PCH})
	set_target_properties(veyon-library-pch PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endif()
