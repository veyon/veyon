include(BuildVeyonPlugin)

build_veyon_plugin(textmessage
	NAME TextMessage
	SOURCES
	TextMessageFeaturePlugin.cpp
	TextMessageDialog.cpp
	TextMessageSanitizer.cpp
	TextMessageDialog.ui
	TextMessageFeaturePlugin.h
	TextMessageDialog.h
	TextMessageSanitizer.h
	textmessage.qrc
	)
