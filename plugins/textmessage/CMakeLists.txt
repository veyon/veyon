INCLUDE(BuildVeyonPlugin)

build_veyon_plugin(textmessage
	TextMessageFeaturePlugin.cpp
	TextMessageDialog.cpp
	TextMessageDialog.ui
	TextMessageFeaturePlugin.h
	TextMessageDialog.h
	textmessage.qrc
)

