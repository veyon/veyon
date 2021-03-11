find_package(Qt5HttpServer)

if(Qt5HttpServer_DIR OR (Qt5Core_VERSION VERSION_GREATER 5.12.0 AND TARGET Qt5::CorePrivate AND TARGET Qt5::NetworkPrivate))

include(BuildVeyonPlugin)

build_veyon_plugin(webapi
	WebApiPlugin.cpp
	WebApiPlugin.h
	WebApiAuthenticationProxy.cpp
	WebApiAuthenticationProxy.h
	WebApiConfiguration.h
	WebApiConfigurationPage.cpp
	WebApiConfigurationPage.h
	WebApiConfigurationPage.ui
	WebApiConnection.cpp
	WebApiConnection.h
	WebApiController.cpp
	WebApiController.h
	WebApiHttpServer.cpp
	WebApiHttpServer.h
	webapi.qrc
	)

set_source_files_properties(WebApiHttpServer.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE SKIP_PRECOMPILE_HEADERS TRUE)

if(Qt5HttpServer_DIR)
	target_link_libraries(webapi Qt5::HttpServer)
else()
	add_subdirectory(qthttpserver)

	target_link_libraries(webapi qthttpserver)
endif()

else()

message(WARNING "Neither Qt5HttpServer nor Qt >= 5.12 with private headers found - omitting WebAPI plugin from build")

endif()
