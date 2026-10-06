include(BuildVeyonApplication)

build_veyon_application(veyon-service
	NAME Service
	SOURCES
	src/main.cpp)

if(VEYON_BUILD_LINUX)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/veyon.service.in ${CMAKE_CURRENT_BINARY_DIR}/veyon.service @ONLY)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/veyon.service DESTINATION ${SYSTEMD_SERVICE_INSTALL_DIR})
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/io.veyon.veyon-service.conf DESTINATION "${CMAKE_INSTALL_DATADIR}/dbus-1/system.d")
endif()
