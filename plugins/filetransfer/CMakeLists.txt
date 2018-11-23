INCLUDE(BuildPlugin)

BUILD_PLUGIN(filetransfer
	FileTransferPlugin.cpp
	FileTransferPlugin.h
	FileTransferController.cpp
	FileTransferController.h
	FileReadThread.cpp
	FileReadThread.h
	filetransfer.qrc
)
