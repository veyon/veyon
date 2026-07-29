include(BuildVeyonPlugin)

build_veyon_plugin(accesscontrol
	NAME AccessControl
	SOURCES
	AccessControlPlugin.cpp AccessControlPlugin.h accesscontrol.qrc
	AccessControlConfigurationPage.cpp AccessControlConfigurationPage.h
	AccessControlConfigurationPage.ui)
