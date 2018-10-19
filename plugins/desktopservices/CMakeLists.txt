INCLUDE(BuildPlugin)

BUILD_PLUGIN(desktopservices
	DesktopServiceObject.cpp
	DesktopServicesConfiguration.cpp
	DesktopServicesConfigurationPage.cpp
	DesktopServicesFeaturePlugin.cpp
	RunProgramDialog.cpp
	DesktopServiceObject.h
	DesktopServicesConfiguration.h
	DesktopServicesConfigurationPage.h
	DesktopServicesFeaturePlugin.h
	RunProgramDialog.h
	desktopservices.qrc
	COTIRE
)

