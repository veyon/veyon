INCLUDE(BuildPlugin)

BUILD_PLUGIN(localbackends
	LocalDataPlugin.cpp
	LocalDataConfigurationPage.cpp
	LocalDataNetworkObjectDirectory.cpp
	MOCFILES
	LocalDataPlugin.h
	LocalDataConfigurationPage.h
	LocalDataNetworkObjectDirectory.h
	FORMS
	LocalDataConfigurationPage.ui
)
