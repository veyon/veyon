include(BuildVeyonPlugin)

if(WITH_TESTS)
	build_veyon_plugin(testing NAME Testing SOURCES TestingCommandLinePlugin.cpp TestingCommandLinePlugin.h)
endif()
