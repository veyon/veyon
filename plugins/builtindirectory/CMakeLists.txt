INCLUDE(BuildVeyonPlugin)

build_veyon_plugin(builtindirectory
	BuiltinDirectoryPlugin.cpp
	BuiltinDirectoryConfigurationPage.cpp
	BuiltinDirectoryConfigurationPage.ui
	BuiltinDirectory.cpp
	BuiltinDirectoryPlugin.h
	BuiltinDirectoryConfiguration.h
	BuiltinDirectoryConfigurationPage.h
	BuiltinDirectory.h
	builtindirectory.qrc
)
