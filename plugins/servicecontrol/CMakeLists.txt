INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

BUILD_PLUGIN(servicecontrol ServiceControlPlugin.cpp MOCFILES ServiceControlPlugin.h)
