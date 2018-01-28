INCLUDE(BuildPlugin)

BUILD_PLUGIN(authkeys
	AuthKeysPlugin.cpp
	AuthKeysManager.cpp
	MOCFILES
	AuthKeysPlugin.h
	AuthKeysManager.h
)
