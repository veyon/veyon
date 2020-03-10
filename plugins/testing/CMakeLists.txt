INCLUDE(BuildVeyonPlugin)

IF(VEYON_DEBUG)
build_veyon_plugin(testing TestingCommandLinePlugin.cpp TestingCommandLinePlugin.h)
ENDIF()
