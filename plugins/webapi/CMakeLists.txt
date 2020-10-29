if(Qt5Core_VERSION VERSION_GREATER 5.12.0 AND TARGET Qt5::CorePrivate AND TARGET Qt5::NetworkPrivate)

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

add_subdirectory(qthttpserver)

target_link_libraries(webapi qthttpserver)

else()

message(WARNING "Qt >= 5.12 or private headers not found - omitting WebAPI plugin from build")

endif()
