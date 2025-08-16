include(BuildVeyonPlugin)

build_veyon_plugin(studentfeatures
	NAME StudentFeatures
	SOURCES
	StudentFeaturesPlugin.cpp StudentFeaturesPlugin.h
	StudentLoginWidget.cpp StudentLoginWidget.h
	StudentAuthManager.cpp StudentAuthManager.h
	StudentInfoSidebar.cpp StudentInfoSidebar.h
	studentfeatures.qrc)
