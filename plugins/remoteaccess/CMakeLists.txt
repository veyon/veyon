include(BuildVeyonPlugin)

build_veyon_plugin(remoteaccess
	NAME RemoteAccess
	SOURCES
	RemoteAccessFeaturePlugin.h
	RemoteAccessFeaturePlugin.cpp
	RemoteAccessWidget.h
	RemoteAccessWidget.cpp
	remoteaccess.qrc)
