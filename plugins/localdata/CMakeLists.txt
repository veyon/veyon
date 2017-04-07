INCLUDE(BuildPlugin)

BUILD_PLUGIN(localdata
	LocalDataPlugin.cpp
	LocalDataConfiguration.cpp
	LocalDataConfigurationPage.cpp
	LocalDataNetworkObjectDirectory.cpp
	MOCFILES
	LocalDataPlugin.h
	LocalDataConfiguration.h
	LocalDataConfigurationPage.h
	LocalDataNetworkObjectDirectory.h
	RESOURCES
	localdata.qrc
	FORMS
	LocalDataConfigurationPage.ui
)
