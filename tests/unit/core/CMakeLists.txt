# Platform-independent core tests (registered outside the Linux-only guard of tests/unit)

if(WITH_TESTS)

	add_executable(VeyonCryptoPrimitivesTest VeyonCryptoPrimitivesTest.cpp)
	set_default_target_properties(VeyonCryptoPrimitivesTest)
	target_link_libraries(VeyonCryptoPrimitivesTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME VeyonCryptoPrimitivesTest COMMAND VeyonCryptoPrimitivesTest)

	# Replaces the OpenSSL memory functions to check zeroisation before release. This only works
	# before OpenSSL allocates anything, which libraries loaded with Qt Network may already do at
	# load time, so SecureBuffer is compiled in directly instead of linking veyon-core.
	add_executable(VeyonCryptoSecureBufferTest
		VeyonCryptoSecureBufferTest.cpp
		${CMAKE_SOURCE_DIR}/core/src/crypto/SecureBuffer.cpp
		)
	set_default_target_properties(VeyonCryptoSecureBufferTest)
	target_include_directories(VeyonCryptoSecureBufferTest PRIVATE ${CMAKE_SOURCE_DIR}/core/src)
	target_compile_definitions(VeyonCryptoSecureBufferTest PRIVATE OPENSSL_API_COMPAT=10101 veyon_core_EXPORTS)
	target_link_libraries(VeyonCryptoSecureBufferTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Core OpenSSL::Crypto)
	add_test(NAME VeyonCryptoSecureBufferTest COMMAND VeyonCryptoSecureBufferTest)

	# uses OpenSSL directly to craft malformed certificates VeyonCrypto never produces
	add_executable(VeyonCryptoPkiTest VeyonCryptoPkiTest.cpp)
	set_default_target_properties(VeyonCryptoPkiTest)
	target_compile_definitions(VeyonCryptoPkiTest PRIVATE OPENSSL_API_COMPAT=10101)
	target_link_libraries(VeyonCryptoPkiTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core veyon-crypto-authority OpenSSL::Crypto)
	add_test(NAME VeyonCryptoPkiTest COMMAND VeyonCryptoPkiTest)

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

	add_executable(ServiceIpcCommandHandlerTest
		ServiceIpcCommandHandlerTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/common/ServiceIpcCommandHandler.cpp
		)
	set_default_target_properties(ServiceIpcCommandHandlerTest)
	target_include_directories(ServiceIpcCommandHandlerTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/common)
	target_link_libraries(ServiceIpcCommandHandlerTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME ServiceIpcCommandHandlerTest COMMAND ServiceIpcCommandHandlerTest)

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
