INCLUDE(BuildPlugin)

BUILD_PLUGIN(remoteaccess RemoteAccessFeaturePlugin.cpp RemoteAccessWidget.cpp
	RemoteAccessFeaturePlugin.h RemoteAccessWidget.h
	remoteaccess.qrc)
