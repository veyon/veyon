include(BuildVeyonPlugin)

build_veyon_plugin(remoteaccess
	NAME RemoteAccess
	SOURCES
	RemoteAccessFeaturePlugin.h
	RemoteAccessFeaturePlugin.cpp
	RemoteAccessSession.h
	RemoteAccessSession.cpp
	remoteaccess.qrc)
