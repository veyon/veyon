include(BuildVeyonPlugin)

build_veyon_plugin(authkeys
	NAME AuthKeys
	SOURCES
	AuthKeysPlugin.cpp
	AuthKeysConfigurationWidget.cpp
	AuthKeysConfigurationWidget.ui
	AuthKeysTableModel.cpp
	AuthKeysManager.cpp
	AuthKeysPlugin.h
	AuthKeysConfigurationWidget.h
	AuthKeysConfiguration.h
	AuthKeysTableModel.h
	AuthKeysManager.h
	)
