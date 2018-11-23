INCLUDE(BuildPlugin)

BUILD_PLUGIN(filetransfer
	FileTransferPlugin.cpp
	FileTransferPlugin.h
	FileTransferController.cpp
	FileTransferController.h
	FileTransferListModel.cpp
	FileTransferListModel.h
	FileTransferDialog.cpp
	FileTransferDialog.h
	FileTransferDialog.ui
	FileTransferUserConfiguration.h
	FileReadThread.cpp
	FileReadThread.h
	filetransfer.qrc
)
