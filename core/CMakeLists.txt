include(WindowsBuildHelpers)

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
	Qt${QT_MAJOR_VERSION}::QuickControls2
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

if(WITH_BUNDLED_LIBVNC)
	target_link_libraries(veyon-core PUBLIC vncclient)
else()
	target_link_libraries(veyon-core PUBLIC LibVNC::LibVNCClient)
endif()

if(VEYON_BUILD_WINDOWS)
	# add Windows Socket library required by libvncclient
	target_link_libraries(veyon-core PRIVATE -lws2_32)
	set_target_properties(veyon-core PROPERTIES PREFIX "")
	add_windows_resources(veyon-core
		NAME VeyonCore
		DESCRIPTION "Veyon Core Library")
	if(NOT WITH_CORE_ONLY)
		install(TARGETS veyon-core RUNTIME DESTINATION ${VEYON_LIB_DIR})
	endif()
else()
	if(NOT WITH_CORE_ONLY)
		install(TARGETS veyon-core LIBRARY DESTINATION ${VEYON_LIB_DIR})
	endif()
endif()

if(VEYON_BUILD_ANDROID)
	target_link_libraries(veyon-core PRIVATE Qt${QT_MAJOR_VERSION}::AndroidExtras)
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
