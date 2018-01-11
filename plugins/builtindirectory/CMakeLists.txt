INCLUDE(BuildPlugin)

BUILD_PLUGIN(builtindirectory
	BuiltinDirectoryPlugin.cpp
	BuiltinDirectoryConfiguration.cpp
	BuiltinDirectoryConfigurationPage.cpp
	BuiltinDirectory.cpp
	MOCFILES
	BuiltinDirectoryPlugin.h
	BuiltinDirectoryConfiguration.h
	BuiltinDirectoryConfigurationPage.h
	BuiltinDirectory.h
	RESOURCES
	builtindirectory.qrc
	FORMS
	BuiltinDirectoryConfigurationPage.ui
	COTIRE
)
