INCLUDE(BuildPlugin)

BUILD_PLUGIN(textmessage
	TextMessageFeaturePlugin.cpp
	TextMessageDialog.cpp
	MOCFILES
	TextMessageFeaturePlugin.h
	TextMessageDialog.h
	RESOURCES
	textmessage.qrc
	FORMS
	TextMessageDialog.ui
)

