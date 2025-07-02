include(BuildVeyonApplication)

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

build_veyon_application(veyon-cli
	NAME CLI
	DESCRIPTION "Veyon Command Line Interface (console version)"
	CONSOLE
	SOURCES ${cli_SOURCES})

target_compile_options(veyon-cli PRIVATE -Wno-date-time)

if(VEYON_BUILD_WINDOWS)
	build_veyon_application(veyon-wcli
		NAME WCLI
		DESCRIPTION "Veyon Command Line Interface (non-console version)"
		SOURCES ${cli_SOURCES})
endif()
