INCLUDE(BuildPlugin)

BUILD_PLUGIN(builtindirectory
	BuiltinDirectoryPlugin.cpp
	BuiltinDirectoryConfiguration.cpp
	BuiltinDirectoryConfigurationPage.cpp
	BuiltinDirectory.cpp
	BuiltinDirectoryPlugin.h
	BuiltinDirectoryConfiguration.h
	BuiltinDirectoryConfigurationPage.h
	BuiltinDirectory.h
	RESOURCES
	builtindirectory.qrc
	COTIRE
)
