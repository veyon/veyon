include(BuildVeyonPlugin)

build_veyon_plugin(remoteaccess
	NAME RemoteAccess
	SOURCES
	RemoteAccessFeaturePlugin.h
	RemoteAccessFeaturePlugin.cpp
	RemoteAccessPage.h
	RemoteAccessPage.cpp
	RemoteAccessWidget.h
	RemoteAccessWidget.cpp
	remoteaccess.qrc)
