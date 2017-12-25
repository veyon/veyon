INCLUDE(BuildPlugin)

BUILD_PLUGIN(external-vnc-server
	ExternalVncServer.cpp
	ExternalVncServerConfiguration.cpp
	ExternalVncServerConfigurationWidget.cpp
	MOCFILES
	ExternalVncServer.h
	ExternalVncServerConfiguration.h
	ExternalVncServerConfigurationWidget.h
	FORMS
	ExternalVncServerConfigurationWidget.ui
	COTIRE
)

