INCLUDE(BuildPlugin)

BUILD_PLUGIN(demo
	DemoFeaturePlugin.cpp
	DemoAuthentication.cpp
	DemoConfigurationPage.cpp
	DemoConfigurationPage.ui
	DemoServer.cpp
	DemoServerConnection.cpp
	DemoServerProtocol.cpp
	DemoClient.cpp
	DemoFeaturePlugin.h
	DemoAuthentication.h
	DemoConfiguration.h
	DemoConfigurationPage.h
	DemoServer.h
	DemoServerConnection.h
	DemoServerProtocol.h
	DemoClient.h
	demo.qrc
)

TARGET_LINK_LIBRARIES(demo ${LZO_LIBRARIES})
