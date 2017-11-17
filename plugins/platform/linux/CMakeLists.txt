INCLUDE(BuildPlugin)

BUILD_PLUGIN(linux-platform
	LinuxPlatformPlugin.cpp
	LinuxCoreFunctions.cpp
	LinuxNetworkFunctions.cpp
	LinuxServiceFunctions.cpp
	LinuxUserInfoFunctions.cpp
	MOCFILES
	LinuxPlatformPlugin.h
	LinuxCoreFunctions.h
	LinuxNetworkFunctions.h
	LinuxServiceFunctions.h
	LinuxUserInfoFunctions.h)

