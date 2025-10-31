include(BuildVeyonPlugin)

build_veyon_plugin(builtin-macvnc-server
	NAME BuiltinMacVncServer
	SOURCES
		BuiltinMacVncServer.cpp
		BuiltinMacVncServer.h
		MacVncConfiguration.h
		MacVncConfigurationWidget.cpp
		MacVncConfigurationWidget.h
		MacVncConfigurationWidget.ui
	)

target_link_libraries(builtin-macvnc-server PRIVATE macvnc Qt${QT_VERSION_MAJOR}::Widgets)
