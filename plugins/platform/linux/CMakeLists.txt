INCLUDE(BuildPlugin)

BUILD_PLUGIN(linux-platform
	LinuxPlatformPlugin.cpp
	LinuxCoreFunctions.cpp
	LinuxNetworkFunctions.cpp
	LinuxUserInfoFunctions.cpp
	MOCFILES
	LinuxPlatformPlugin.h
	LinuxCoreFunctions.h
	LinuxNetworkFunctions.h
	LinuxUserInfoFunctions.h)

