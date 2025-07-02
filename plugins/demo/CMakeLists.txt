include(BuildVeyonPlugin)

build_veyon_plugin(demo
	NAME Demo
	SOURCES
	DemoFeaturePlugin.cpp
	DemoConfigurationPage.cpp
	DemoConfigurationPage.ui
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
