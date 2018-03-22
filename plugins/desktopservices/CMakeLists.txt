INCLUDE(BuildPlugin)

BUILD_PLUGIN(desktopservices
	DesktopServiceObject.cpp
	DesktopServicesConfiguration.cpp
	DesktopServicesConfigurationPage.cpp
	DesktopServicesFeaturePlugin.cpp
	RunProgramDialog.cpp
	MOCFILES
	DesktopServiceObject.h
	DesktopServicesConfiguration.h
	DesktopServicesConfigurationPage.h
	DesktopServicesFeaturePlugin.h
	RunProgramDialog.h
	RESOURCES
	desktopservices.qrc
	FORMS
	DesktopServicesConfigurationPage.ui
	RunProgramDialog.ui
	COTIRE
)

