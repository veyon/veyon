INCLUDE(BuildPlugin)

BUILD_PLUGIN(authkeys
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
