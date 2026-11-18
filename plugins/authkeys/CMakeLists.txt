include(BuildVeyonPlugin)

build_veyon_plugin(authkeys
	NAME AuthKeys
	SOURCES
	QuickConfiguration.cpp
	QuickConfigurationController.cpp
	QuickConfigurationController.h
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
