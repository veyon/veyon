INCLUDE(BuildPlugin)

find_package(OpenSSL REQUIRED)

BUILD_PLUGIN(authsmartcard
	AuthSmartcardPlugin.cpp
	AuthSmartcardPlugin.h
	AuthSmartcardConfiguration.h
	AuthSmartcardDialog.cpp
	AuthSmartcardDialog.h
	AuthSmartcardDialog.ui
)

target_link_libraries( authsmartcard
	${OPENSSL_LIBRARIES}
	)