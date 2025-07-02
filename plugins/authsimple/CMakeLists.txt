include(BuildVeyonPlugin)

build_veyon_plugin(authsimple
	NAME AuthSimple
	SOURCES
	AuthSimplePlugin.cpp
	AuthSimplePlugin.h
	AuthSimpleConfiguration.h
	AuthSimpleDialog.cpp
	AuthSimpleDialog.h
	AuthSimpleDialog.ui
	)
