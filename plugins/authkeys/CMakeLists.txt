INCLUDE(BuildVeyonPlugin)

build_veyon_plugin(authkeys
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
