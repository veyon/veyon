INCLUDE(BuildPlugin)

BUILD_PLUGIN(linux-platform
	LinuxPlatformPlugin.cpp
	LinuxCoreFunctions.cpp
	LinuxInputDeviceFunctions.cpp
	LinuxNetworkFunctions.cpp
	LinuxServiceFunctions.cpp
	LinuxUserFunctions.cpp
	MOCFILES
	LinuxPlatformPlugin.h
	LinuxCoreFunctions.h
	LinuxInputDeviceFunctions.h
	LinuxNetworkFunctions.h
	LinuxServiceFunctions.h
	LinuxUserFunctions.h
	COTIRE
)

TARGET_LINK_LIBRARIES(linux-platform ${X11_LIBRARIES})

