INCLUDE(BuildPlugin)

BUILD_PLUGIN(linux-platform LinuxPlatformPlugin.cpp LinuxNetworkFunctions.cpp LinuxUserSessionFunctions.cpp MOCFILES LinuxPlatformPlugin.h LinuxNetworkFunctions.h LinuxUserSessionFunctions.h)

