INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

BUILD_PLUGIN(screenshot ScreenshotFeaturePlugin.cpp MOCFILES ScreenshotFeaturePlugin.h RESOURCES screenshot.qrc)
