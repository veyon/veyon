set(SOURCES
	ChatPlugin.cpp
	ChatMasterDialog.cpp
	ChatStudentDialog.cpp
)

set(HEADERS
	ChatPlugin.h
	ChatMasterDialog.h
	ChatStudentDialog.h
)

set(UIS
	ChatMasterDialog.ui
	ChatStudentDialog.ui
)

build_veyon_plugin(chat)
