INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

BUILD_PLUGIN(powercontrol PowerControlFeaturePlugin.cpp PowerControl.cpp MOCFILES PowerControlFeaturePlugin.h RESOURCES powercontrol.qrc)

