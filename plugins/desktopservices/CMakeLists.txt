INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

BUILD_PLUGIN(desktopservices DesktopServicesFeaturePlugin.cpp RunProgramDialog.cpp MOCFILES DesktopServicesFeaturePlugin.h RunProgramDialog.h RESOURCES desktopservices.qrc FORMS RunProgramDialog.ui COTIRE)

