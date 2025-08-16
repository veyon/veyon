include(BuildVeyonPlugin)

build_veyon_plugin(studentfeatures
	NAME StudentFeatures
	SOURCES
	StudentFeaturesPlugin.cpp StudentFeaturesPlugin.h
	StudentLoginWidget.cpp StudentLoginWidget.h
	StudentAuthManager.cpp StudentAuthManager.h
	StudentInfoSidebar.cpp StudentInfoSidebar.h
	StudentFeaturesConfiguration.h
	StudentFeaturesConfigurationPage.cpp StudentFeaturesConfigurationPage.h StudentFeaturesConfigurationPage.ui
	studentfeatures.qrc)
