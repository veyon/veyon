include(BuildPlugin)

if(VEYON_DEBUG)
build_plugin(testing TestingCommandLinePlugin.cpp TestingCommandLinePlugin.h)
endif()
