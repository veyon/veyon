include(BuildVeyonApplication)
include(WindowsBuildHelpers)

set(cli_SOURCES
	src/main.cpp
	src/ConfigCommands.cpp
	src/ConfigCommands.h
	src/FeatureCommands.cpp
	src/FeatureCommands.h
	src/PluginCommands.cpp
	src/PluginCommands.h
	src/ServiceControlCommands.cpp
	src/ServiceControlCommands.h
	src/ShellCommands.cpp
	src/ShellCommands.h
	)

build_veyon_application(veyon-cli ${cli_SOURCES})

target_compile_options(veyon-cli PRIVATE -Wno-date-time)

add_windows_resource(veyon-cli ${CMAKE_CURRENT_BINARY_DIR}/veyon-cli.rc)
make_console_app(veyon-cli)

if(VEYON_BUILD_WIN32)
	build_veyon_application(veyon-wcli ${cli_SOURCES})

	add_windows_resource(veyon-wcli ${CMAKE_CURRENT_BINARY_DIR}/veyon-wcli.rc)
	make_graphical_app(veyon-wcli)
endif()
