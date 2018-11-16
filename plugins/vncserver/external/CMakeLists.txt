INCLUDE(BuildPlugin)

BUILD_PLUGIN(external-vnc-server
	ExternalVncServer.cpp
	ExternalVncServerConfigurationWidget.cpp
	ExternalVncServerConfigurationWidget.ui
	ExternalVncServer.h
	ExternalVncServerConfiguration.h
	ExternalVncServerConfigurationWidget.h
)

