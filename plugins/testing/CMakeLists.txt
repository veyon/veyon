include(BuildVeyonPlugin)

if(VEYON_DEBUG)
	build_veyon_plugin(testing
		NAME Testing
		SOURCES
		TestingCommandLinePlugin.cpp
		TestingCommandLinePlugin.h
		)
endif()
