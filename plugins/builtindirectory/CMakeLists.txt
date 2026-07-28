include(BuildVeyonPlugin)

build_veyon_plugin(builtindirectory
	NAME BuiltinDirectory
	SOURCES
	BuiltinDirectoryPlugin.cpp
	BuiltinDirectoryConfigurationPage.cpp
	BuiltinDirectoryConfigurationPage.ui
	NetworkDiscoveryDialog.cpp
	NetworkDiscoveryDialog.ui
	BuiltinDirectory.cpp
	BuiltinDirectoryPlugin.h
	BuiltinDirectoryConfiguration.h
	BuiltinDirectoryConfigurationPage.h
	NetworkDiscoveryDialog.h
	BuiltinDirectory.h
	builtindirectory.qrc
	)
