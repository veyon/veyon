include(BuildVeyonPlugin)

build_veyon_plugin(authlogon
	NAME AuthLogon
	SOURCES
	AuthLogonPlugin.cpp
	AuthLogonPlugin.h
	AuthLogonDialog.cpp
	AuthLogonDialog.h
	AuthLogonDialog.ui
	)
