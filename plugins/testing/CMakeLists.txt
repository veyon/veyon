include(BuildVeyonPlugin)

if(VEYON_DEBUG)
	build_veyon_plugin(testing TestingCommandLinePlugin.cpp TestingCommandLinePlugin.h)
endif()
