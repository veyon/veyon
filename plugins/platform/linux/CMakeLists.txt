INCLUDE(BuildPlugin)

BUILD_PLUGIN(linux-platform LinuxPlatformPlugin.cpp LinuxNetworkFunctions.cpp LinuxUserInfoFunctions.cpp MOCFILES LinuxPlatformPlugin.h LinuxNetworkFunctions.h LinuxUserInfoFunctions.h)

