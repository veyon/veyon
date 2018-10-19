INCLUDE(BuildPlugin)

BUILD_PLUGIN(remoteaccess RemoteAccessFeaturePlugin.cpp RemoteAccessWidget.cpp
	RemoteAccessFeaturePlugin.h RemoteAccessWidget.h
	RESOURCES remoteaccess.qrc)
