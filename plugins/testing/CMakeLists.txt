INCLUDE(BuildPlugin)

IF(VEYON_DEBUG)
BUILD_PLUGIN(testing TestingCommandLinePlugin.cpp TestingCommandLinePlugin.h)
ENDIF()
