include(BuildVeyonApplication)
include(WindowsBuildHelpers)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(cli_SOURCES src/main.cpp)

build_veyon_application(veyon-cli ${cli_SOURCES})
target_link_libraries(veyon-cli veyon-core Qt5::Network)

add_windows_resource(veyon-cli ${CMAKE_CURRENT_BINARY_DIR}/veyon-cli.rc)
make_console_app(veyon-cli)

if(VEYON_BUILD_WIN32)
	build_veyon_application(veyon-wcli ${cli_SOURCES})
	target_link_libraries(veyon-wcli veyon-core Qt5::Network)

	add_windows_resource(veyon-wcli ${CMAKE_CURRENT_BINARY_DIR}/veyon-wcli.rc)
	make_graphical_app(veyon-wcli)
endif()
