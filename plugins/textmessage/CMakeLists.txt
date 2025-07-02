include(BuildVeyonPlugin)

build_veyon_plugin(textmessage
	NAME TextMessage
	SOURCES
	TextMessageFeaturePlugin.cpp
	TextMessageDialog.cpp
	TextMessageDialog.ui
	TextMessageFeaturePlugin.h
	TextMessageDialog.h
	textmessage.qrc
	)
