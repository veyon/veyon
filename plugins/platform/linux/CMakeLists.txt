INCLUDE(BuildPlugin)

BUILD_PLUGIN(linux-platform
	LinuxPlatformPlugin.cpp
	LinuxCoreFunctions.cpp
	LinuxNetworkFunctions.cpp
	LinuxServiceFunctions.cpp
	LinuxUserFunctions.cpp
	MOCFILES
	LinuxPlatformPlugin.h
	LinuxCoreFunctions.h
	LinuxNetworkFunctions.h
	LinuxServiceFunctions.h
	LinuxUserFunctions.h
	COTIRE
)
