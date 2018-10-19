INCLUDE(BuildPlugin)

BUILD_PLUGIN(external-vnc-server
	ExternalVncServer.cpp
	ExternalVncServerConfiguration.cpp
	ExternalVncServerConfigurationWidget.cpp
	ExternalVncServer.h
	ExternalVncServerConfiguration.h
	ExternalVncServerConfigurationWidget.h
	FORMS
	ExternalVncServerConfigurationWidget.ui
	COTIRE
)

