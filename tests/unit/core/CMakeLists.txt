# Platform-independent core tests (registered outside the Linux-only guard of tests/unit)

if(WITH_TESTS)

	# the VeyonCrypto tests link veyon-crypto only, so that they also verify that it builds and links
	# without veyon-core (06-pki.md §2)
	add_executable(VeyonCryptoPrimitivesTest VeyonCryptoPrimitivesTest.cpp)
	set_default_target_properties(VeyonCryptoPrimitivesTest)
	target_link_libraries(VeyonCryptoPrimitivesTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-crypto)
	add_test(NAME VeyonCryptoPrimitivesTest COMMAND VeyonCryptoPrimitivesTest)

	# Replaces the OpenSSL memory functions to check zeroisation before release. This only works
	# before OpenSSL allocates anything, which libraries loaded with Qt Network may already do at
	# load time, so SecureBuffer is compiled in directly instead of linking veyon-crypto.
	add_executable(VeyonCryptoSecureBufferTest
		VeyonCryptoSecureBufferTest.cpp
		${CMAKE_SOURCE_DIR}/crypto/src/CryptoLogging.cpp
		${CMAKE_SOURCE_DIR}/crypto/src/SecureBuffer.cpp
		)
	set_default_target_properties(VeyonCryptoSecureBufferTest)
	target_include_directories(VeyonCryptoSecureBufferTest PRIVATE ${CMAKE_SOURCE_DIR}/crypto/src)
	target_compile_definitions(VeyonCryptoSecureBufferTest PRIVATE OPENSSL_API_COMPAT=10101 veyon_crypto_EXPORTS)
	target_link_libraries(VeyonCryptoSecureBufferTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Core OpenSSL::Crypto)
	add_test(NAME VeyonCryptoSecureBufferTest COMMAND VeyonCryptoSecureBufferTest)

	# uses OpenSSL directly to craft malformed certificates VeyonCrypto never produces
	add_executable(VeyonCryptoPkiTest VeyonCryptoPkiTest.cpp)
	set_default_target_properties(VeyonCryptoPkiTest)
	target_compile_definitions(VeyonCryptoPkiTest PRIVATE OPENSSL_API_COMPAT=10101)
	target_link_libraries(VeyonCryptoPkiTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-crypto-authority OpenSSL::Crypto)
	add_test(NAME VeyonCryptoPkiTest COMMAND VeyonCryptoPkiTest)

	# known-answer tests against fixtures produced with Veyon 4.x (QCA); OpenSSL is used directly to
	# create signatures and ciphertexts with paddings VeyonCrypto never produces
	add_executable(VeyonCryptoLegacyRsaTest VeyonCryptoLegacyRsaTest.cpp VeyonCryptoLegacyRsaTest.qrc)
	set_default_target_properties(VeyonCryptoLegacyRsaTest)
	target_compile_definitions(VeyonCryptoLegacyRsaTest PRIVATE OPENSSL_API_COMPAT=10101)
	target_link_libraries(VeyonCryptoLegacyRsaTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-crypto OpenSSL::Crypto)
	add_test(NAME VeyonCryptoLegacyRsaTest COMMAND VeyonCryptoLegacyRsaTest)

	# PkiPublicApi; test certificates are crafted with OpenSSL directly (PkiTestCertificates.h), so
	# that the tests do not need veyon-crypto-authority (capability boundary, 06-pki.md §1)
	add_executable(TlsPeerVerifierTest TlsPeerVerifierTest.cpp PkiTestCertificates.h)
	set_default_target_properties(TlsPeerVerifierTest)
	target_compile_definitions(TlsPeerVerifierTest PRIVATE OPENSSL_API_COMPAT=10101)
	target_link_libraries(TlsPeerVerifierTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core OpenSSL::Crypto)
	add_test(NAME TlsPeerVerifierTest COMMAND TlsPeerVerifierTest)

	add_executable(TrustBundleTest TrustBundleTest.cpp PkiTestCertificates.h)
	set_default_target_properties(TrustBundleTest)
	target_compile_definitions(TrustBundleTest PRIVATE OPENSSL_API_COMPAT=10101)
	target_link_libraries(TrustBundleTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core OpenSSL::Crypto)
	add_test(NAME TrustBundleTest COMMAND TrustBundleTest)

	add_executable(IpcChannelTest IpcChannelTest.cpp)
	set_default_target_properties(IpcChannelTest)
	target_link_libraries(IpcChannelTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME IpcChannelTest COMMAND IpcChannelTest)

	# server-side cache and request collection of the IdentifyUser feature (MonitoringMode)
	add_executable(UserIdentificationCacheTest UserIdentificationCacheTest.cpp)
	set_default_target_properties(UserIdentificationCacheTest)
	target_link_libraries(UserIdentificationCacheTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME UserIdentificationCacheTest COMMAND UserIdentificationCacheTest)

	add_executable(AuthenticationLimiterTest
		AuthenticationLimiterTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/common/AuthenticationLimiter.cpp
		)
	set_default_target_properties(AuthenticationLimiterTest)
	target_include_directories(AuthenticationLimiterTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/common)
	target_link_libraries(AuthenticationLimiterTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME AuthenticationLimiterTest COMMAND AuthenticationLimiterTest)

	# credential check of master and configurator through the local server (02-ipc.md §6b)
	add_executable(LocalCredentialsVerifierTest LocalCredentialsVerifierTest.cpp)
	set_default_target_properties(LocalCredentialsVerifierTest)
	target_link_libraries(LocalCredentialsVerifierTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Network veyon-core)
	add_test(NAME LocalCredentialsVerifierTest COMMAND LocalCredentialsVerifierTest)

	add_executable(ServiceIpcCommandHandlerTest
		ServiceIpcCommandHandlerTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/common/AuthenticationLimiter.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/common/IpcAuthenticationRequest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/common/ServiceIpcCommandHandler.cpp
		)
	set_default_target_properties(ServiceIpcCommandHandlerTest)
	target_include_directories(ServiceIpcCommandHandlerTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/common)
	target_link_libraries(ServiceIpcCommandHandlerTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME ServiceIpcCommandHandlerTest COMMAND ServiceIpcCommandHandlerTest)

	# directory and user-groups broker (04-directory-broker.md §7)
	add_executable(DirectoryBrokerTest DirectoryBrokerTest.cpp DummyDirectoryPlugin.h)
	set_default_target_properties(DirectoryBrokerTest)
	target_link_libraries(DirectoryBrokerTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME DirectoryBrokerTest COMMAND DirectoryBrokerTest)

	# fail-closed access control decisions on directory failures
	add_executable(DirectoryAccessControlTest DirectoryAccessControlTest.cpp)
	set_default_target_properties(DirectoryAccessControlTest)
	target_link_libraries(DirectoryAccessControlTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME DirectoryAccessControlTest COMMAND DirectoryAccessControlTest)

	# grace period and re-check wiring of the session server
	add_executable(ServerAccessControlManagerTest
		ServerAccessControlManagerTest.cpp
		${CMAKE_SOURCE_DIR}/server/src/ServerAccessControlManager.cpp
		${CMAKE_SOURCE_DIR}/server/src/ServerAccessControlManager.h
		)
	set_default_target_properties(ServerAccessControlManagerTest)
	target_include_directories(ServerAccessControlManagerTest PRIVATE ${CMAKE_SOURCE_DIR}/server/src)
	target_link_libraries(ServerAccessControlManagerTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME ServerAccessControlManagerTest COMMAND ServerAccessControlManagerTest)

	# SecretStore, SecretReference, legacy secret migration and encrypted secret exports
	# (03-secretstore-config.md §6); the platform backends are tested separately
	add_executable(SecretStoreTest SecretStoreTest.cpp)
	set_default_target_properties(SecretStoreTest)
	target_compile_definitions(SecretStoreTest PRIVATE VEYON_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
	target_link_libraries(SecretStoreTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME SecretStoreTest COMMAND SecretStoreTest)

	# registry of secret properties declared by core and the plugins (03-secretstore-config.md §3a)
	include(BuildVeyonPlugin)
	add_library(secret-properties-test-plugin MODULE secret-properties/SecretPropertiesTestPlugin.cpp)
	target_link_libraries(secret-properties-test-plugin PRIVATE veyon-core)
	add_veyon_plugin_build_version(secret-properties-test-plugin)

	add_executable(SecretPropertyRegistryTest SecretPropertyRegistryTest.cpp)
	set_default_target_properties(SecretPropertyRegistryTest)
	add_dependencies(SecretPropertyRegistryTest secret-properties-test-plugin)
	target_compile_definitions(SecretPropertyRegistryTest PRIVATE
		SECRET_PROPERTIES_TEST_PLUGIN_FILE_PATH="$<TARGET_FILE:secret-properties-test-plugin>")
	target_link_libraries(SecretPropertyRegistryTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME SecretPropertyRegistryTest COMMAND SecretPropertyRegistryTest)

	# veyon-cli reading of secrets and writing of exports
	add_executable(SecretInputOutputTest
		SecretInputOutputTest.cpp
		${CMAKE_SOURCE_DIR}/cli/src/SecretInputOutput.cpp
		)
	set_default_target_properties(SecretInputOutputTest)
	target_include_directories(SecretInputOutputTest PRIVATE ${CMAKE_SOURCE_DIR}/cli/src)
	target_link_libraries(SecretInputOutputTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME SecretInputOutputTest COMMAND SecretInputOutputTest)

	# LdapClient never binds without its bind password (no unauthenticated/anonymous fallback)
	if(TARGET kldap-light)
		add_executable(LdapClientBindTest
			LdapClientBindTest.cpp
			${CMAKE_SOURCE_DIR}/plugins/ldap/LdapClient.cpp
			${CMAKE_SOURCE_DIR}/plugins/ldap/LdapConfiguration.cpp
			)
		set_default_target_properties(LdapClientBindTest)
		target_include_directories(LdapClientBindTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/ldap
			${CMAKE_SOURCE_DIR}/plugins/vncserver/external ${Ldap_INCLUDE_DIRS})
		target_link_libraries(LdapClientBindTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core kldap-light)
		add_test(NAME LdapClientBindTest COMMAND LdapClientBindTest)
		if(VEYON_BUILD_WINDOWS)
			# kldap-light.dll is not next to the test executable
			set_tests_properties(LdapClientBindTest PROPERTIES
				ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:kldap-light>")
		endif()

		# search status: result codes, timeouts, lost connections (fail-closed queries)
		add_executable(LdapClientQueryTest
			LdapClientQueryTest.cpp
			${CMAKE_SOURCE_DIR}/plugins/ldap/LdapClient.cpp
			${CMAKE_SOURCE_DIR}/plugins/ldap/LdapConfiguration.cpp
			${CMAKE_SOURCE_DIR}/plugins/ldap/LdapDirectory.cpp
			${CMAKE_SOURCE_DIR}/plugins/ldap/LdapNetworkObjectDirectory.cpp
			)
		set_default_target_properties(LdapClientQueryTest)
		target_include_directories(LdapClientQueryTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/ldap ${Ldap_INCLUDE_DIRS})
		target_link_libraries(LdapClientQueryTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Network veyon-core kldap-light)
		add_test(NAME LdapClientQueryTest COMMAND LdapClientQueryTest)
		if(VEYON_BUILD_WINDOWS)
			set_tests_properties(LdapClientQueryTest PROPERTIES
				ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:kldap-light>")
		endif()
	endif()

	# platform plugin of the build tree for tests that create a VeyonCore instance
	if(VEYON_BUILD_WINDOWS)
		set(VEYON_TEST_PLATFORM_PLUGIN windows-platform)
	else()
		set(VEYON_TEST_PLATFORM_PLUGIN linux-platform)
	endif()

	# redaction of sensitive feature arguments in logs (05-tls.md §7a), with the plugins of the build tree
	add_executable(FeatureMessageRedactionTest FeatureMessageRedactionTest.cpp)
	set_default_target_properties(FeatureMessageRedactionTest)
	target_link_libraries(FeatureMessageRedactionTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Widgets veyon-core)
	add_dependencies(FeatureMessageRedactionTest ${VEYON_TEST_PLATFORM_PLUGIN} usersessioncontrol demo remoteaccess filetransfer)
	add_test(NAME FeatureMessageRedactionTest COMMAND FeatureMessageRedactionTest)
	set_tests_properties(FeatureMessageRedactionTest PROPERTIES LABELS platform-plugin)

	# debug output of the WebAPI plugin (05-tls.md §7a)
	if(WITH_WEBAPI)
		add_executable(WebApiLogRedactionTest WebApiLogRedactionTest.cpp)
		set_default_target_properties(WebApiLogRedactionTest)
		target_include_directories(WebApiLogRedactionTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/webapi)
		target_link_libraries(WebApiLogRedactionTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Concurrent veyon-core)
		add_test(NAME WebApiLogRedactionTest COMMAND WebApiLogRedactionTest)
	endif()

	# pre-auth hardening of the master (VeyonConnection) and the server (VncServerProtocol,
	# ServerAuthenticationManager) and 4.11 interoperability (05-tls.md §6)
	add_executable(VeyonAuthenticationTest
		VeyonAuthenticationTest.cpp
		${CMAKE_SOURCE_DIR}/server/src/ServerAuthenticationManager.cpp
		)
	set_default_target_properties(VeyonAuthenticationTest)
	target_include_directories(VeyonAuthenticationTest PRIVATE ${CMAKE_SOURCE_DIR}/server/src)
	target_link_libraries(VeyonAuthenticationTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Network veyon-core)
	add_dependencies(VeyonAuthenticationTest ${VEYON_TEST_PLATFORM_PLUGIN})
	add_test(NAME VeyonAuthenticationTest COMMAND VeyonAuthenticationTest)
	set_tests_properties(VeyonAuthenticationTest PROPERTIES LABELS platform-plugin)

	# logon credentials in the service data blob (05-tls.md §7)
	add_executable(PersistentLogonCredentialsTest
		PersistentLogonCredentialsTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/common/PersistentLogonCredentials.cpp
		)
	set_default_target_properties(PersistentLogonCredentialsTest)
	target_include_directories(PersistentLogonCredentialsTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/common)
	target_link_libraries(PersistentLogonCredentialsTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_dependencies(PersistentLogonCredentialsTest ${VEYON_TEST_PLATFORM_PLUGIN})
	add_test(NAME PersistentLogonCredentialsTest COMMAND PersistentLogonCredentialsTest)
	set_tests_properties(PersistentLogonCredentialsTest PROPERTIES LABELS platform-plugin)

	if(VEYON_BUILD_WINDOWS)
		# DACLs of the IPC endpoint pipes (02-ipc.md §1, §6a)
		add_executable(IpcPipeSecurityTest IpcPipeSecurityTest.cpp)
		set_default_target_properties(IpcPipeSecurityTest)
		target_link_libraries(IpcPipeSecurityTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core advapi32)
		add_test(NAME IpcPipeSecurityTest COMMAND IpcPipeSecurityTest)

		# log file directories and the protected DACL of the system log file directory (WP23)
		add_executable(WindowsLogFileDirectoryTest
			WindowsLogFileDirectoryTest.cpp
			${CMAKE_SOURCE_DIR}/plugins/platform/windows/WindowsLogFileDirectory.cpp
			)
		set_default_target_properties(WindowsLogFileDirectoryTest)
		target_include_directories(WindowsLogFileDirectoryTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/windows)
		target_link_libraries(WindowsLogFileDirectoryTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core advapi32)
		add_test(NAME WindowsLogFileDirectoryTest COMMAND WindowsLogFileDirectoryTest)
	endif()

endif()
