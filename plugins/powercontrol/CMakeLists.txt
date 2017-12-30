INCLUDE(BuildPlugin)

BUILD_PLUGIN(powercontrol
	PowerControlFeaturePlugin.cpp
	MOCFILES
	PowerControlFeaturePlugin.h
	RESOURCES
	powercontrol.qrc
)
