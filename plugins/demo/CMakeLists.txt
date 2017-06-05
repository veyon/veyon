INCLUDE(BuildPlugin)

BUILD_PLUGIN(demo DemoFeaturePlugin.cpp DemoServer.cpp DemoServerConnection.cpp DemoServerProtocol.cpp DemoClient.cpp MOCFILES DemoFeaturePlugin.h DemoServer.h DemoServerConnection.h DemoServerProtocol.h DemoClient.h RESOURCES demo.qrc)

TARGET_LINK_LIBRARIES(demo ${LZO_LIBRARIES})
