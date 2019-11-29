include(BuildVeyonApplication)
include(WindowsBuildHelpers)

set(cli_SOURCES
	src/main.cpp
	src/PluginsCommands.cpp
	src/PluginsCommands.h
	)

build_veyon_application(veyon-cli ${cli_SOURCES})

add_windows_resource(veyon-cli ${CMAKE_CURRENT_BINARY_DIR}/veyon-cli.rc)
make_console_app(veyon-cli)

if(VEYON_BUILD_WIN32)
	build_veyon_application(veyon-wcli ${cli_SOURCES})

	add_windows_resource(veyon-wcli ${CMAKE_CURRENT_BINARY_DIR}/veyon-wcli.rc)
	make_graphical_app(veyon-wcli)
endif()
