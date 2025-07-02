include(BuildVeyonPlugin)

build_veyon_plugin(authkeys
	NAME AuthKeys
	SOURCES
	AuthKeysPlugin.cpp
	AuthKeysConfigurationPage.cpp
	AuthKeysConfigurationPage.ui
	AuthKeysTableModel.cpp
	AuthKeysManager.cpp
	AuthKeysPlugin.h
	AuthKeysConfigurationPage.h
	AuthKeysTableModel.h
	AuthKeysManager.h
	)
