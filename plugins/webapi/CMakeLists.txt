if(WITH_QT6)
	find_package(Qt6 COMPONENTS HttpServer)
else()
	find_package(Qt5HttpServer)
endif()

if(Qt6HttpServer_DIR OR Qt5HttpServer_DIR OR (Qt5Core_VERSION VERSION_GREATER 5.12.0 AND TARGET Qt5::CorePrivate AND TARGET Qt5::NetworkPrivate))

	include(BuildVeyonPlugin)

	build_veyon_plugin(webapi
		NAME WebApi
		SOURCES
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

	if(Qt6HttpServer_DIR)
		target_link_libraries(webapi PRIVATE Qt6::HttpServer)
	elseif(Qt5HttpServer_DIR)
		target_link_libraries(webapi PRIVATE Qt5::HttpServer)
	else()
		add_subdirectory(qthttpserver)

		target_link_libraries(webapi PRIVATE qthttpserver)
	endif()

else()

	message(WARNING "Neither Qt6HttpServer nor Qt5HttpServer nor Qt >= 5.12 with private headers found - omitting WebAPI plugin from build")

endif()
