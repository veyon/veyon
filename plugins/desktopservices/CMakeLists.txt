include(BuildVeyonPlugin)

build_veyon_plugin(desktopservices
	NAME DesktopServices
	SOURCES
	QuickConfiguration.cpp
	QuickConfigurationController.cpp
	QuickConfigurationController.h
	DesktopServiceObject.cpp
	DesktopServicesConfigurationPage.cpp
	DesktopServicesConfigurationPage.ui
	DesktopServicesFeaturePlugin.cpp
	StartAppDialog.cpp
	StartAppDialog.h
	StartAppDialog.ui
	OpenWebsiteDialog.cpp
	OpenWebsiteDialog.h
	OpenWebsiteDialog.ui
	DesktopServiceObject.h
	DesktopServicesConfiguration.h
	DesktopServicesConfigurationPage.h
	DesktopServicesFeaturePlugin.h
	desktopservices.qrc
	)
