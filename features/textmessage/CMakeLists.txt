INCLUDE(BuildPlugin)

BUILD_PLUGIN(textmessage TextMessageFeature.cpp TextMessageDialog.cpp MOCFILES TextMessageFeature.h TextMessageDialog.h RESOURCES textmessage.qrc FORMS TextMessageDialog.ui)

