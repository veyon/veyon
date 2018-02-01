INCLUDE(BuildPlugin)

BUILD_PLUGIN(authkeys
	AuthKeysPlugin.cpp
	AuthKeysConfigurationPage.cpp
	AuthKeysTableModel.cpp
	AuthKeysManager.cpp
	MOCFILES
	AuthKeysPlugin.h
	AuthKeysConfigurationPage.h
	AuthKeysTableModel.h
	AuthKeysManager.h
	FORMS
	AuthKeysConfigurationPage.ui
	RESOURCES
	authkeys.qrc
	COTIRE
)
