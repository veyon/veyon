include(BuildVeyonPlugin)

build_veyon_plugin(authkeys
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
