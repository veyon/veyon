INCLUDE(BuildVeyonPlugin)

build_veyon_plugin(external-vnc-server
	ExternalVncServer.cpp
	ExternalVncServerConfigurationWidget.cpp
	ExternalVncServerConfigurationWidget.ui
	ExternalVncServer.h
	ExternalVncServerConfiguration.h
	ExternalVncServerConfigurationWidget.h
)

