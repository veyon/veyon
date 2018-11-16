INCLUDE(BuildPlugin)

BUILD_PLUGIN(desktopservices
	DesktopServiceObject.cpp
	DesktopServicesConfigurationPage.cpp
	DesktopServicesConfigurationPage.ui
	DesktopServicesFeaturePlugin.cpp
	RunProgramDialog.cpp
	RunProgramDialog.ui
	DesktopServiceObject.h
	DesktopServicesConfiguration.h
	DesktopServicesConfigurationPage.h
	DesktopServicesFeaturePlugin.h
	RunProgramDialog.h
	desktopservices.qrc
)

