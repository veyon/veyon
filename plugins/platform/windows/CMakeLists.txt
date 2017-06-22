INCLUDE(BuildPlugin)

BUILD_PLUGIN(windows-platform WindowsPlatformPlugin.cpp WindowsNetworkFunctions.cpp WindowsUserSessionFunctions.cpp MOCFILES WindowsPlatformPlugin.h WindowsNetworkFunctions.h WindowsUserSessionFunctions.h)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32)
