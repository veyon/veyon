INCLUDE(BuildPlugin)

BUILD_PLUGIN(demo
	DemoFeaturePlugin.cpp
	DemoConfiguration.cpp
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
	RESOURCES demo.qrc
	COTIRE
)

TARGET_LINK_LIBRARIES(demo ${LZO_LIBRARIES})
