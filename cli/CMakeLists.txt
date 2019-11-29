INCLUDE(BuildApplication)
INCLUDE(WindowsBuildHelpers)

SET(cli_SOURCES
	src/main.cpp
	src/PluginsCommands.cpp
)

BUILD_APPLICATION(veyon-cli ${cli_SOURCES})

ADD_WINDOWS_RESOURCE(veyon-cli ${CMAKE_CURRENT_BINARY_DIR}/veyon-cli.rc)
MAKE_CONSOLE_APP(veyon-cli)

IF(VEYON_BUILD_WIN32)
BUILD_APPLICATION(veyon-wcli ${cli_SOURCES})

ADD_WINDOWS_RESOURCE(veyon-wcli ${CMAKE_CURRENT_BINARY_DIR}/veyon-wcli.rc)
MAKE_GRAPHICAL_APP(veyon-wcli)
ENDIF()
