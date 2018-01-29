INCLUDE(BuildPlugin)

BUILD_PLUGIN(authkeys
	AuthKeysPlugin.cpp
	AuthKeysConfigurationPage.cpp
	AuthKeysManager.cpp
	MOCFILES
	AuthKeysPlugin.h
	AuthKeysConfigurationPage.h
	AuthKeysManager.h
	FORMS
	AuthKeysConfigurationPage.ui
	RESOURCES
	authkeys.qrc
	COTIRE
)
