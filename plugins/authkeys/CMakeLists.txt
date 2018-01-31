INCLUDE(BuildPlugin)

BUILD_PLUGIN(authkeys
	AuthKeysPlugin.cpp
	AuthKeysConfigurationPage.cpp
	AuthKeysListModel.cpp
	AuthKeysManager.cpp
	MOCFILES
	AuthKeysPlugin.h
	AuthKeysConfigurationPage.h
	AuthKeysListModel.h
	AuthKeysManager.h
	FORMS
	AuthKeysConfigurationPage.ui
	RESOURCES
	authkeys.qrc
	COTIRE
)
