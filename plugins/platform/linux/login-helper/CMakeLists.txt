find_package(Qt${QT_MAJOR_VERSION} COMPONENTS Core DBus Network REQUIRED)
find_package(PAM REQUIRED)

add_executable(veyon-login-helper
	main.cpp
	LoginInterfaces.h
	LoginPeer.h
	LoginSession.cpp LoginSession.h
	LightDmSession.cpp LightDmSession.h
	LoginService.cpp LoginService.h
	ProcessIdentity.cpp ProcessIdentity.h
	LightDmAdapter.cpp LightDmAdapter.h
	LightDmGreeter.cpp LightDmGreeter.h
	FdChannel.cpp FdChannel.h
)
set_default_target_properties(veyon-login-helper)
target_include_directories(veyon-login-helper PRIVATE ${PAM_INCLUDE_DIR})
target_link_libraries(veyon-login-helper PRIVATE
	Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::DBus Qt${QT_MAJOR_VERSION}::Network)

install(TARGETS veyon-login-helper RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
foreach(file veyon-login-helper.service io.veyon.LoginHelper.service)
	configure_file(${file}.in ${CMAKE_CURRENT_BINARY_DIR}/${file} @ONLY)
endforeach()
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/veyon-login-helper.service DESTINATION ${SYSTEMD_SERVICE_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/io.veyon.LoginHelper.service DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services)
install(FILES io.veyon.LoginHelper.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d)
configure_file(veyon-lightdm-greeter.in ${CMAKE_CURRENT_BINARY_DIR}/veyon-lightdm-greeter @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/veyon-lightdm-greeter DESTINATION ${CMAKE_INSTALL_BINDIR})
configure_file(90-veyon-login.conf.in ${CMAKE_CURRENT_BINARY_DIR}/90-veyon-login.conf @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/90-veyon-login.conf
	DESTINATION ${CMAKE_INSTALL_DATADIR}/lightdm/lightdm.conf.d)
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DATADIR}/veyon/login-helper)
