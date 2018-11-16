INCLUDE(BuildPlugin)

BUILD_PLUGIN(textmessage
	TextMessageFeaturePlugin.cpp
	TextMessageDialog.cpp
	TextMessageDialog.ui
	TextMessageFeaturePlugin.h
	TextMessageDialog.h
	textmessage.qrc
)

