# veyon-crypto-authority: certificate issuance primitives (Veyon::Crypto::CertificateAuthority).
#
# Capability boundary (06-pki.md §1): a separate static library that only the CA-host authority
# code may link. It must never be linked into veyon-crypto, veyon-core or
# veyon-master/-server/-worker; the top-level CMakeLists.txt verifies this at configure time.

add_library(veyon-crypto-authority STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/CertificateAuthority.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/CertificateAuthority.h
	)

set_default_target_properties(veyon-crypto-authority)
set_target_properties(veyon-crypto-authority PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(WITH_LTO AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# the archive is created with plain ar, so it needs regular object code next to the LTO data
	target_compile_options(veyon-crypto-authority PRIVATE -ffat-lto-objects)
endif()

target_include_directories(veyon-crypto-authority PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(veyon-crypto-authority PRIVATE OPENSSL_API_COMPAT=10101)
target_link_libraries(veyon-crypto-authority PUBLIC veyon-crypto PRIVATE OpenSSL::Crypto)
