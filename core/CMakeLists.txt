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

	set_source_files_properties(${libvncclient_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unused-variable -fvisibility=default")
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

if(VEYON_BUILD_WIN32)
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

if(VEYON_BUILD_WIN32)
	target_compile_definitions(veyon-core PUBLIC _WIN32_WINNT=0x0602)
endif()

if(WITH_PCH)
	target_precompile_headers(veyon-core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/PrecompiledHeader.h")

	add_library(veyon-pch STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/PrecompiledHeader.h")

	if(${CMAKE_VERSION} VERSION_GREATER "3.17.5")
		file(GENERATE
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cxx
			CONTENT "/*empty file*/")
		set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cxx PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE SKIP_AUTOGEN TRUE)
		target_sources(veyon-pch PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cxx)
	endif()
	set_default_target_properties(veyon-pch)
	if(WITH_QT6)
		target_link_libraries(veyon-pch PUBLIC Qt6::Core Qt6::Concurrent Qt6::Network Qt6::Widgets)
	else()
		target_link_libraries(veyon-pch PUBLIC Qt5::Core Qt5::Concurrent Qt5::Network Qt5::Widgets)
	endif()
	target_precompile_headers(veyon-pch PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/PrecompiledHeader.h")
	if(VEYON_BUILD_WIN32)
		target_compile_definitions(veyon-pch PUBLIC _WIN32_WINNT=0x0602)
	endif()
endif()
