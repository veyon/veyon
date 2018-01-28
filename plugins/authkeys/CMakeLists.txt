INCLUDE(BuildPlugin)

BUILD_PLUGIN(authkeys
	AuthKeysCommandLinePlugin.cpp
	AuthKeysManager.cpp
	MOCFILES
	AuthKeysCommandLinePlugin.h
	AuthKeysManager.h
)
