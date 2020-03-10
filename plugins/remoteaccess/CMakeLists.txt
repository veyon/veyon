INCLUDE(BuildVeyonPlugin)

build_veyon_plugin(remoteaccess RemoteAccessFeaturePlugin.cpp RemoteAccessWidget.cpp
	RemoteAccessFeaturePlugin.h RemoteAccessWidget.h
	remoteaccess.qrc)
