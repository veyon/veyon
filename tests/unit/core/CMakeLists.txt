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

endif()
