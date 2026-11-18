include(BuildVeyonPlugin)

build_veyon_plugin(external-vnc-server
	NAME ExternalVncServer
	SOURCES
	QuickConfiguration.cpp
	ExternalVncServer.cpp
	ExternalVncServerConfigurationWidget.cpp
	ExternalVncServerConfigurationWidget.ui
	ExternalVncServer.h
	ExternalVncServerConfiguration.h
	ExternalVncServerConfigurationWidget.h
	)
