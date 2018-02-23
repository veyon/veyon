INCLUDE(BuildPlugin)

BUILD_PLUGIN(screenshot
	ScreenshotFeaturePlugin.cpp
	MOCFILES
	ScreenshotFeaturePlugin.h
	RESOURCES
	screenshot.qrc
)
