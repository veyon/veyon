# veyon-crypto: VeyonCrypto, the OpenSSL-based crypto layer of Veyon (06-pki.md §2). It does not
# depend on veyon-core: it must not include any Veyon core headers (checked in CI by
# .ci/common/check-openssl-version-guards.sh).

include(WindowsBuildHelpers)

set(veyoncrypto_SOURCES
	src/Aead.cpp
	src/Aead.h
	src/Certificate.cpp
	src/Certificate.h
	src/CertificateRequest.cpp
	src/CertificateRequest.h
	src/ConstantTime.cpp
	src/ConstantTime.h
	src/CryptoError.cpp
	src/CryptoError.h
	src/CryptoLogging.cpp
	src/CryptoLogging.h
	src/CryptoPrivate.h
	src/Hash.cpp
	src/Hash.h
	src/Hkdf.cpp
	src/Hkdf.h
	src/Hmac.cpp
	src/Hmac.h
	src/LegacyRsa.cpp
	src/LegacyRsa.h
	src/LegacyRsaPrivateKey.cpp
	src/LegacyRsaPrivateKey.h
	src/LegacyRsaPublicKey.cpp
	src/LegacyRsaPublicKey.h
	src/OpenSslCompat.cpp
	src/OpenSslCompat.h
	src/OpenSslUtils.cpp
	src/OpenSslUtils.h
	src/PasswordKdf.cpp
	src/PasswordKdf.h
	src/PrivateKey.cpp
	src/PrivateKey.h
	src/PublicKey.cpp
	src/PublicKey.h
	src/Random.cpp
	src/Random.h
	src/SecureBuffer.cpp
	src/SecureBuffer.h
	src/VeyonCryptoExport.h
	)

add_library(veyon-crypto SHARED ${veyoncrypto_SOURCES})

set_default_target_properties(veyon-crypto)

target_include_directories(veyon-crypto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Qt Network for the QSslCertificate/QHostAddress conversions of Certificate
target_link_libraries(veyon-crypto PUBLIC
	Qt${QT_MAJOR_VERSION}::Core
	Qt${QT_MAJOR_VERSION}::Network
	PRIVATE
	OpenSSL::Crypto
	)

# only the OpenSSL API subset common to 1.1.1, 3.x and 4.x (06-pki.md §2)
target_compile_definitions(veyon-crypto PRIVATE OPENSSL_API_COMPAT=10101)

if(VEYON_BUILD_WINDOWS)
	set_target_properties(veyon-crypto PROPERTIES PREFIX "")
	add_windows_resources(veyon-crypto
		NAME VeyonCrypto
		DESCRIPTION "Veyon Crypto Library")
	if(NOT WITH_CORE_ONLY)
		install(TARGETS veyon-crypto RUNTIME DESTINATION ${VEYON_LIB_DIR})
	endif()
else()
	if(NOT WITH_CORE_ONLY)
		install(TARGETS veyon-crypto LIBRARY DESTINATION ${VEYON_LIB_DIR})
	endif()
endif()

# CA primitives in a separate library outside veyon-crypto and veyon-core (capability boundary, 06-pki.md §1)
add_subdirectory(src/authority)
