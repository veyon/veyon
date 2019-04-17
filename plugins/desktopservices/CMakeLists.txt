INCLUDE(BuildPlugin)

BUILD_PLUGIN(desktopservices
	DesktopServiceObject.cpp
	DesktopServicesConfigurationPage.cpp
	DesktopServicesConfigurationPage.ui
	DesktopServicesFeaturePlugin.cpp
	RunProgramDialog.cpp
	RunProgramDialog.h
	RunProgramDialog.ui
	OpenWebsiteDialog.cpp
	OpenWebsiteDialog.h
	OpenWebsiteDialog.ui
	DesktopServiceObject.h
	DesktopServicesConfiguration.h
	DesktopServicesConfigurationPage.h
	DesktopServicesFeaturePlugin.h
	desktopservices.qrc
)

