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

if(NOT WITH_BUILTIN_LIBVNC)
	find_package(LibVNCClient 0.9.14)
endif()

if(LibVNCClient_FOUND)
	set_property(GLOBAL PROPERTY HAVE_LIBVNCCLIENT TRUE)
else()
	message(WARNING "Performing internal build of LibVNCClient which requires additional development packages")
	include(LibVNCServerIntegration)
	set(libvncclient_SOURCES
		${libvncserver_DIR}/src/libvncclient/cursor.c
		${libvncserver_DIR}/src/libvncclient/listen.c
		${libvncserver_DIR}/src/libvncclient/rfbclient.c
		${libvncserver_DIR}/src/libvncclient/sockets.c
		${libvncserver_DIR}/src/libvncclient/tls_openssl.c
		${libvncserver_DIR}/src/libvncclient/vncviewer.c
		${libvncserver_DIR}/src/common/crypto_openssl.c
		${libvncserver_DIR}/src/common/sockets.c
		${libvncserver_DIR}/src/common/turbojpeg.c)

	set_source_files_properties(${libvncclient_SOURCES} PROPERTIES
		COMPILE_FLAGS "-Wno-unused-function -Wno-unused-variable -fvisibility=default -Wno-deprecated-declarations"
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
	)

target_link_libraries(veyon-core PUBLIC
	Qt${QT_MAJOR_VERSION}::Concurrent
	Qt${QT_MAJOR_VERSION}::Gui
	Qt${QT_MAJOR_VERSION}::Network
	Qt${QT_MAJOR_VERSION}::Widgets
	qca-qt${QT_MAJOR_VERSION}
	)

if(WITH_QT6)
	# required by qca-qt6
	target_link_libraries(veyon-core PUBLIC Qt6::Core5Compat)
endif()

if(WITH_TESTS)
	target_link_libraries(veyon-core PUBLIC Qt${QT_MAJOR_VERSION}::Test)
endif()

target_link_libraries(veyon-core PUBLIC OpenSSL::SSL)

if(LibVNCClient_FOUND)
	target_link_libraries(veyon-core PRIVATE LibVNC::LibVNCClient)
else()
	target_include_directories(veyon-core PRIVATE
		${ZLIB_INCLUDE_DIR}
		${JPEG_INCLUDE_DIR}
		${LZO_INCLUDE_DIR}
		)
	target_include_directories(veyon-core PUBLIC
		${libvncserver_DIR}/src/common/
		${libvncserver_DIR}/include
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

if(VEYON_BUILD_WINDOWS)
	target_compile_definitions(veyon-core PUBLIC _WIN32_WINNT=0x0602)
endif()

if(WITH_PCH)
	set(PCH ${CMAKE_CURRENT_SOURCE_DIR}/src/PrecompiledHeader.h)
	target_precompile_headers(veyon-core PRIVATE ${PCH})

	add_pch_target(veyon-application-pch ${PCH})
	set_target_properties(veyon-application-pch PROPERTIES COMPILE_OPTIONS "${CMAKE_COMPILE_OPTIONS_PIE}")
	set_target_properties(veyon-application-pch PROPERTIES LINK_OPTIONS "${CMAKE_LINK_OPTIONS_PIE}")

	add_pch_target(veyon-library-pch ${PCH})
	set_target_properties(veyon-library-pch PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endif()
