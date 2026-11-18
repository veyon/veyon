include(BuildVeyonFuzzer)

set(PLATFORM_COMMON_DIR ${CMAKE_SOURCE_DIR}/plugins/platform/common)

build_veyon_fuzzer(ipcauthenticationrequest
	main.cpp
	${PLATFORM_COMMON_DIR}/AuthenticationLimiter.cpp
	${PLATFORM_COMMON_DIR}/IpcAuthenticationRequest.cpp
	${PLATFORM_COMMON_DIR}/ServiceIpcCommandHandler.cpp
)
target_include_directories(ipcauthenticationrequest PRIVATE ${PLATFORM_COMMON_DIR})
