INCLUDE(BuildPlugin)

BUILD_PLUGIN(localdata
	LocalDataPlugin.cpp
	LocalDataConfigurationPage.cpp
	LocalDataNetworkObjectDirectory.cpp
	MOCFILES
	LocalDataPlugin.h
	LocalDataConfigurationPage.h
	LocalDataNetworkObjectDirectory.h
	RESOURCES
	localdata.qrc
	FORMS
	LocalDataConfigurationPage.ui
)
