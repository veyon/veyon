// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QTimer>

#include "ComputerControlInterface.h"

class FileReadThread;
class FileTransferPlugin;

// clazy:excludeall=ctor-missing-parent-argument
class FileTransferController : public QObject
{
	Q_OBJECT
public:
	enum Flag {
		Transfer = 0x00,
		OpenFilesInApplication = 0x01,
		OpenTransferFolder = 0x02,
		OverwriteExistingFiles = 0x04,
	};
	Q_DECLARE_FLAGS(Flags, Flag)
	Q_FLAG(Flags)

	explicit FileTransferController(FileTransferPlugin* plugin);
	~FileTransferController() override;

	void setFiles(const QStringList& files);
	void setInterfaces(const ComputerControlInterfaceList& interfaces);
	void setFlags(Flags flags);
	void setDestinationDirectory(const QString& destDir);

	void start();
	void stop();

	const QStringList& files() const;

	int currentFileIndex() const;

	bool isRunning() const;

Q_SIGNALS:
	void errorOccurred(const QString& message);
	void filesChanged();
	void progressChanged(int progress);
	void started();
	void finished();

private:
	enum FileState {
		FileStateOpen,
		FileStateTransferring,
		FileStateFinished
	};

	void process();

	bool openFile();
	bool transferFile();
	void finishFile();

	void updateProgress();

	bool allQueuesEmpty();

	static constexpr int ProcessInterval = 25;
	static constexpr int ChunkSize = 256 * 1024;

	FileTransferPlugin* m_plugin;

	int m_currentFileIndex;
	QUuid m_currentTransferId;
	QStringList m_files;
	Flags m_flags;
	ComputerControlInterfaceList m_interfaces;

	FileReadThread* m_fileReadThread;

	FileState m_fileState;

	QTimer m_processTimer;

	QString m_destinationDirectory;
};
