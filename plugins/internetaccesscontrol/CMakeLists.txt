ADD_SUBDIRECTORY(backends)

INCLUDE(BuildPlugin)

BUILD_PLUGIN(internetaccesscontrol
	InternetAccessControlPlugin.cpp
	InternetAccessControlConfiguration.cpp
	InternetAccessControlConfigurationPage.cpp
	InternetAccessControlPlugin.h
	InternetAccessControlConfiguration.h
	InternetAccessControlConfigurationPage.h
	RESOURCES
	internetaccesscontrol.qrc
	COTIRE
)

