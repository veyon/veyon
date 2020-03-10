include(BuildVeyonPlugin)

build_veyon_plugin(authkeys
	AuthKeysPlugin.cpp
	AuthKeysConfigurationDialog.cpp
	AuthKeysConfigurationDialog.ui
	AuthKeysTableModel.cpp
	AuthKeysManager.cpp
	AuthKeysPlugin.h
	AuthKeysConfigurationDialog.h
	AuthKeysConfiguration.h
	AuthKeysTableModel.h
	AuthKeysManager.h
)
