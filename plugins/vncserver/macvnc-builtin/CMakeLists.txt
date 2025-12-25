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

target_include_directories(builtin-macvnc-server PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/macvnc)
target_link_libraries(builtin-macvnc-server PRIVATE macvnc_lib Qt${QT_VERSION_MAJOR}::Widgets)
