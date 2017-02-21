INCLUDE(BuildPlugin)

BUILD_PLUGIN(demo DemoFeaturePlugin.cpp DemoServer.cpp DemoClient.cpp MOCFILES DemoFeaturePlugin.h DemoServer.h DemoClient.h RESOURCES demo.qrc)

TARGET_LINK_LIBRARIES(demo ${LZO_LIBRARIES})
