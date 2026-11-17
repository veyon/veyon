include(WindowsBuildHelpers)

set(veyoncore_SOURCES
	src/AboutDialog.cpp
	src/AboutDialog.h
	src/AboutDialog.ui
	src/AccessControlProvider.cpp
	src/AccessControlProvider.h
	src/AccessControlRule.cpp
	src/AccessControlRule.h
	src/AuthenticationCredentials.cpp
	src/AuthenticationCredentials.h
	src/AuthenticationProxy.h
	src/BuiltinFeatures.cpp
	src/BuiltinFeatures.h
	src/CommandLineIO.cpp
	src/CommandLineIO.h
	src/CommandLinePluginInterface.h
	src/Computer.cpp
	src/Computer.h
	src/ComputerControlInterface.cpp
	src/ComputerControlInterface.h
	src/ComputerListModel.cpp
	src/ComputerListModel.h
	src/ConfigurationManager.cpp
	src/ConfigurationManager.h
	src/ConfigurationPage.cpp
	src/ConfigurationPage.h
	src/ConfigurationPagePluginInterface.h
	src/ConnectionLimiter.cpp
	src/ConnectionLimiter.h
	src/CryptoCore.cpp
	src/CryptoCore.h
	src/d3des.c
	src/d3des.h
	src/DesktopAccessDialog.cpp
	src/DesktopAccessDialog.h
	src/EnumHelper.h
	src/Feature.h
	src/FeatureManager.cpp
	src/FeatureManager.h
	src/FeatureMessage.cpp
	src/FeatureMessage.h
	src/FeatureProviderInterface.h
	src/FeatureWorkerManager.cpp
	src/FeatureWorkerManager.h
	src/Filesystem.cpp
	src/Filesystem.h
	src/FileSystemBrowser.cpp
	src/FileSystemBrowser.h
	src/HashList.h
	src/HostAddress.cpp
	src/HostAddress.h
	src/IpcChannel.cpp
	src/IpcChannel.h
	src/IpcClient.cpp
	src/IpcClient.h
	src/IpcConnection.cpp
	src/IpcConnection.h
	src/IpcEndpointCredentials.cpp
	src/IpcEndpointCredentials.h
	src/IpcMessage.cpp
	src/IpcMessage.h
	src/IpcProtocol.h
	src/IpcServer.cpp
	src/IpcServer.h
	src/IpcServerPlatform.h
	src/KeyboardShortcutTrapper.h
	src/Lockable.h
	src/LockingPointer.h
	src/LockWidget.cpp
	src/LockWidget.h
	src/Logger.cpp
	src/Logger.h
	src/MessageContext.h
	src/MonitoringMode.cpp
	src/MonitoringMode.h
	src/NetworkObject.cpp
	src/NetworkObject.h
	src/NetworkObjectDirectory.cpp
	src/NetworkObjectDirectory.h
	src/NetworkObjectDirectoryManager.cpp
	src/NetworkObjectDirectoryManager.h
	src/NetworkObjectDirectoryPluginInterface.h
	src/NetworkObjectModel.h
	src/ObjectManager.h
	src/PasswordDialog.cpp
	src/PasswordDialog.h
	src/PasswordDialog.ui
	src/PlatformCoreFunctions.h
	src/PlatformFilesystemFunctions.h
	src/PlatformInputDeviceFunctions.h
	src/PlatformNetworkFunctions.h
	src/PlatformPluginInterface.h
	src/PlatformPluginManager.cpp
	src/PlatformPluginManager.h
	src/PlatformServiceFunctions.h
	src/PlatformSessionFunctions.h
	src/PlatformUserFunctions.h
	src/Plugin.h
	src/PluginInterface.h
	src/PluginManager.cpp
	src/PluginManager.h
	src/PrecompiledHeader.h
	src/ProcessHardening.h
	src/ProcessHelper.cpp
	src/ProcessHelper.h
	src/RfbVeyonAuth.h
	src/Screenshot.cpp
	src/Screenshot.h
	src/ServiceControl.cpp
	src/ServiceControl.h
	src/SocketDevice.h
	src/StandardInputReader.cpp
	src/StandardInputReader.h
	src/StandardInputReaderPlatform.h
	src/SystemTrayIcon.cpp
	src/SystemTrayIcon.h
	src/Toast.cpp
	src/Toast.h
	src/ToolButton.cpp
	src/ToolButton.h
	src/TranslationLoader.cpp
	src/TranslationLoader.h
	src/UserGroupsBackendInterface.h
	src/UserGroupsBackendManager.cpp
	src/UserGroupsBackendManager.h
	src/VariantArrayMessage.cpp
	src/VariantArrayMessage.h
	src/VariantStream.cpp
	src/VariantStream.h
	src/VeyonConfiguration.cpp
	src/VeyonConfiguration.h
	src/VeyonConfigurationProperties.h
	src/VeyonConnection.cpp
	src/VeyonConnection.h
	src/VeyonCore.cpp
	src/VeyonCore.h
	src/VeyonMasterInterface.h
	src/VeyonServerInterface.h
	src/VeyonServiceControl.cpp
	src/VeyonServiceControl.h
	src/VeyonWorkerInterface.h
	src/VncClientProtocol.cpp
	src/VncClientProtocol.h
	src/VncConnection.cpp
	src/VncConnection.h
	src/VncConnectionConfiguration.h
	src/VncEvents.cpp
	src/VncEvents.h
	src/VncFeatureMessageEvent.cpp
	src/VncFeatureMessageEvent.h
	src/VncServerClient.h
	src/VncServerPluginInterface.h
	src/VncServerProtocol.cpp
	src/VncServerProtocol.h
	src/VncView.cpp
	src/VncView.h
	src/VncViewWidget.cpp
	src/VncViewWidget.h
	src/Configuration/JsonStore.cpp
	src/Configuration/JsonStore.h
	src/Configuration/LocalStore.cpp
	src/Configuration/LocalStore.h
	src/Configuration/Object.cpp
	src/Configuration/Object.h
	src/Configuration/Password.cpp
	src/Configuration/Password.h
	src/Configuration/Property.cpp
	src/Configuration/Property.h
	src/Configuration/Proxy.cpp
	src/Configuration/Proxy.h
	src/Configuration/Store.h
	src/Configuration/UiMapping.cpp
	src/Configuration/UiMapping.h
	src/crypto/Aead.cpp
	src/crypto/Aead.h
	src/crypto/Certificate.cpp
	src/crypto/Certificate.h
	src/crypto/CertificateRequest.cpp
	src/crypto/CertificateRequest.h
	src/crypto/ConstantTime.cpp
	src/crypto/ConstantTime.h
	src/crypto/CryptoError.cpp
	src/crypto/CryptoError.h
	src/crypto/CryptoPrivate.h
	src/crypto/Hash.cpp
	src/crypto/Hash.h
	src/crypto/Hkdf.cpp
	src/crypto/Hkdf.h
	src/crypto/Hmac.cpp
	src/crypto/Hmac.h
	src/crypto/OpenSslCompat.cpp
	src/crypto/OpenSslCompat.h
	src/crypto/OpenSslUtils.cpp
	src/crypto/OpenSslUtils.h
	src/crypto/PasswordKdf.cpp
	src/crypto/PasswordKdf.h
	src/crypto/PrivateKey.cpp
	src/crypto/PrivateKey.h
	src/crypto/PublicKey.cpp
	src/crypto/PublicKey.h
	src/crypto/Random.cpp
	src/crypto/Random.h
	src/crypto/SecureBuffer.cpp
	src/crypto/SecureBuffer.h
	src/pki/PkiPublicApi.h
	src/pki/TlsClientPolicy.cpp
	src/pki/TlsClientPolicy.h
	src/pki/TlsConfiguration.cpp
	src/pki/TlsConfiguration.h
	src/pki/TlsError.cpp
	src/pki/TlsError.h
	src/pki/TlsPeerVerifier.cpp
	src/pki/TlsPeerVerifier.h
	src/pki/TrustBundle.cpp
	src/pki/TrustBundle.h
	)

# platform parts of core classes (Qt layout, e.g. IpcServer.cpp + IpcServerUnix.cpp / IpcServerWindows.cpp)
if(VEYON_BUILD_WINDOWS)
	list(APPEND veyoncore_SOURCES
		src/IpcConnectionWindows.cpp
		src/IpcServerWindows.cpp
		src/LoggerWindows.cpp
		src/ProcessHardeningWindows.cpp
		src/StandardInputReaderWindows.cpp
		)
else()
	list(APPEND veyoncore_SOURCES
		src/IpcConnectionUnix.cpp
		src/IpcServerUnix.cpp
		src/LoggerUnix.cpp
		src/ProcessHardeningUnix.cpp
		src/StandardInputReaderUnix.cpp
		)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/builddata.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc)
set(core_RESOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/resources/core.qrc
	${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc
	)

add_library(veyon-core SHARED ${veyoncore_SOURCES} ${core_RESOURCES} ${libvncclient_SOURCES})

target_compile_definitions(veyon-core PRIVATE XK_KOREAN)

set_default_target_properties(veyon-core)

target_compile_options(veyon-core PRIVATE -Wno-parentheses)

target_include_directories(veyon-core PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_BINARY_DIR}/src
	)

target_link_libraries(veyon-core PUBLIC
	Qt${QT_MAJOR_VERSION}::Concurrent
	Qt${QT_MAJOR_VERSION}::Gui
	Qt${QT_MAJOR_VERSION}::Network
	Qt${QT_MAJOR_VERSION}::Widgets
	qca-qt${QT_MAJOR_VERSION}
	)

if(WITH_QT6)
	# required by qca-qt6
	target_link_libraries(veyon-core PUBLIC Qt6::Core5Compat)
endif()

if(WITH_TESTS)
	target_link_libraries(veyon-core PUBLIC Qt${QT_MAJOR_VERSION}::Test)
endif()

target_link_libraries(veyon-core PUBLIC OpenSSL::SSL)

# VeyonCrypto (src/crypto) uses the OpenSSL API subset common to 1.1.1, 3.x and 4.x (06-pki.md §2)
target_compile_definitions(veyon-core PRIVATE OPENSSL_API_COMPAT=10101)

# CA primitives in a separate library outside veyon-core (capability boundary, 06-pki.md §1)
add_subdirectory(src/crypto/authority)

if(WITH_BUNDLED_LIBVNC)
	target_link_libraries(veyon-core PUBLIC vncclient)
else()
	target_link_libraries(veyon-core PUBLIC LibVNC::LibVNCClient)
endif()

if(VEYON_BUILD_WINDOWS)
	# add Windows Socket library required by libvncclient
	target_link_libraries(veyon-core PRIVATE -lws2_32)
	set_target_properties(veyon-core PROPERTIES PREFIX "")
	add_windows_resources(veyon-core
		NAME VeyonCore
		DESCRIPTION "Veyon Core Library")
	if(NOT WITH_CORE_ONLY)
		install(TARGETS veyon-core RUNTIME DESTINATION ${VEYON_LIB_DIR})
	endif()
else()
	if(NOT WITH_CORE_ONLY)
		install(TARGETS veyon-core LIBRARY DESTINATION ${VEYON_LIB_DIR})
	endif()
endif()

if(VEYON_BUILD_WINDOWS)
	target_compile_definitions(veyon-core PUBLIC _WIN32_WINNT=0x0602)
endif()

if(WITH_PCH)
	set(PCH ${CMAKE_CURRENT_SOURCE_DIR}/src/PrecompiledHeader.h)
	target_precompile_headers(veyon-core PRIVATE ${PCH})

	add_pch_target(veyon-application-pch ${PCH})
	set_target_properties(veyon-application-pch PROPERTIES COMPILE_OPTIONS "${CMAKE_COMPILE_OPTIONS_PIE}")
	set_target_properties(veyon-application-pch PROPERTIES LINK_OPTIONS "${CMAKE_LINK_OPTIONS_PIE}")

	add_pch_target(veyon-library-pch ${PCH})
	set_target_properties(veyon-library-pch PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
endif()
