include(BuildVeyonPlugin)

build_veyon_plugin(screenshot
	NAME Screenshot
	SOURCES
	ScreenshotFeaturePlugin.cpp
	ScreenshotFeaturePlugin.h
	ScreenshotListModel.cpp
	ScreenshotListModel.h
	screenshot.qrc
	)
