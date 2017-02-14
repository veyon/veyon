INCLUDE(BuildPlugin)

BUILD_PLUGIN(remoteaccess RemoteAccessFeaturePlugin.cpp RemoteAccessWidget.cpp
	MOCFILES RemoteAccessFeaturePlugin.h RemoteAccessWidget.h
	RESOURCES remoteaccess.qrc)
