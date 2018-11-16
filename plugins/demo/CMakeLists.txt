INCLUDE(BuildPlugin)

BUILD_PLUGIN(demo
	DemoFeaturePlugin.cpp
	DemoConfigurationPage.cpp
	DemoServer.cpp
	DemoServerConnection.cpp
	DemoServerProtocol.cpp
	DemoClient.cpp
	DemoFeaturePlugin.h
	DemoConfiguration.h
	DemoConfigurationPage.h
	DemoServer.h
	DemoServerConnection.h
	DemoServerProtocol.h
	DemoClient.h
	demo.qrc
)

TARGET_LINK_LIBRARIES(demo ${LZO_LIBRARIES})
