// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later
#include "FileCollectDialog.h"
#include "FileCollectController.h"
#include "Filesystem.h"
#include "QuickUi.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

FileCollectDialog::FileCollectDialog(FileCollectController* controller, QWindow* parent) :
	QObject(nullptr), m_controller(controller), m_parentWindow(parent)
{
	connect(controller, &FileCollectController::overallProgressChanged, this, &FileCollectDialog::changed);
	connect(controller, &FileCollectController::collectionsChanged, this, &FileCollectDialog::changed);
	connect(controller, &FileCollectController::collectionChanged, this, [this]() { Q_EMIT changed(); });
	connect(controller, &FileCollectController::collectionInitialized, this, [this]() { Q_EMIT changed(); });
	connect(controller, &FileCollectController::started, this, [this]() { m_completed = false; Q_EMIT changed(); });
	connect(controller, &FileCollectController::finished, this, [this]() { m_completed = true; Q_EMIT changed(); });
}
FileCollectDialog::~FileCollectDialog() { delete m_engine; }
void FileCollectDialog::open()
{
	m_engine = QuickUi::createEngine(this);
	m_engine->rootContext()->setContextProperty(QStringLiteral("collectDialog"), this);
	m_engine->load(QUrl(QStringLiteral("qrc:/filetransfer/qml/CollectWindow.qml")));
	if (m_engine->rootObjects().isEmpty()) {
		QuickUi::showError(tr("Collect files"), tr("The file collection window could not be loaded."), m_parentWindow);
		Q_EMIT finished();
			return;
		}
	auto window = qobject_cast<QWindow*>(m_engine->rootObjects().first());
	if (window) window->setTransientParent(m_parentWindow);
}
QString FileCollectDialog::sourceDirectory() const { return m_controller ? m_controller->collectSourceDirectory() : QString(); }
QString FileCollectDialog::filePattern() const { return m_controller ? m_controller->filePattern() : QString(); }
QString FileCollectDialog::destinationDirectory() const { return m_controller ? m_controller->destinationDirectory() : QString(); }
int FileCollectDialog::progress() const { return m_controller ? m_controller->overallProgress() : 0; }
bool FileCollectDialog::running() const { return m_controller && m_controller->isRunning(); }
QVariantList FileCollectDialog::collections() const
{
	QVariantList rows;
	if (!m_controller) return rows;
	for (const auto& collection : m_controller->collections()) {
		QVariantList files;
		for (int i = 0; i < collection.files.size(); ++i) {
			const int value = i < collection.processedFilesCount ? 100 :
				(i == collection.processedFilesCount ? collection.currentFileProgress() : 0);
			files.append(QVariantMap{{QStringLiteral("name"), collection.files.at(i)}, {QStringLiteral("progress"), value}});
		}
		QString status;
		switch (collection.state) {
		case FileCollection::State::Initializing: status = tr("Waiting for file list"); break;
		case FileCollection::State::Finished: status = tr("Finished"); break;
		case FileCollection::State::FileTransferRunning: status = tr("Receiving"); break;
		case FileCollection::State::ReadyForNextFileTransfer: status = tr("Ready"); break;
		default: status = tr("Waiting"); break;
		}
		if (m_completed && collection.state != FileCollection::State::Finished) status = tr("Stopped");
		rows.append(QVariantMap{{QStringLiteral("name"), collection.name}, {QStringLiteral("status"), status},
			{QStringLiteral("progress"), collection.progress()}, {QStringLiteral("files"), files},
			{QStringLiteral("fileCount"), int(collection.files.size())}});
	}
	return rows;
}
void FileCollectDialog::browseDestination()
{
	if (!m_controller || running()) return;
	const auto result = QuickUi::execDialog(QStringLiteral("folder"),
		{{QStringLiteral("title"), tr("Select destination directory")}, {QStringLiteral("folder"), destinationDirectory()}}, m_parentWindow);
	if (result.value(QStringLiteral("accepted")).toBool()) {
		m_controller->setDestinationDirectory(result.value(QStringLiteral("path")).toString());
		Q_EMIT changed();
	}
}
void FileCollectDialog::start(const QString& source, const QString& pattern, int subfolders)
{
	if (!m_controller || running()) return;
	switch (m_controller->collectionDirectory()) {
	case FileCollectController::CollectionDirectory::PromptUserForName: {
		const auto result = QuickUi::execDialog(QStringLiteral("input"),
			{{QStringLiteral("title"), tr("Enter collection name")},
				{QStringLiteral("label"), tr("Please enter a name for this file collection:")}}, m_parentWindow);
		const auto name = result.value(QStringLiteral("value")).toString().trimmed();
		if (!result.value(QStringLiteral("accepted")).toBool() || name.isEmpty()) return;
		if (name == QStringLiteral(".") || name == QStringLiteral("..") || name.contains(u'/') || name.contains(u'\\')) {
			QuickUi::showError(tr("Collection name"), tr("Please enter a name without directory separators."), m_parentWindow);
			return;
		}
		m_controller->setCollectionName(name);
		break;
	}
	case FileCollectController::CollectionDirectory::DateTimeSubdirectory:
		m_controller->setCollectionName(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmm")));
		break;
	default: break;
	}
	m_controller->setCollectSourceDirectory(source.trimmed());
	m_controller->setFilePattern(pattern.trimmed());
	m_controller->setSubfolderHandling(static_cast<FileCollectController::SubfolderHandling>(qBound(0, subfolders, 2)));
	if (!VeyonCore::filesystem().ensurePathExists(m_controller->outputDirectory())) {
		QuickUi::showError(tr("Output directory creation failed"),
			tr("The output directory \"%1\" could not be created. Check the directory and its permissions.").arg(m_controller->outputDirectory()), m_parentWindow);
		return;
	}
	if (m_completed)
	{
		ComputerControlInterfaceList interfaces;
		for (auto it = m_controller->collections().cbegin(); it != m_controller->collections().cend(); ++it)
			interfaces.append(it.key());
		m_controller->setInterfaces(interfaces);
	}
	if (m_controller->collections().isEmpty())
	{
		QuickUi::showError(tr("Collect files"), tr("No connected computers with logged on users are available."), m_parentWindow);
		return;
	}
	m_controller->start();
}
void FileCollectDialog::openOutputDirectory()
{
	if (m_controller) QDesktopServices::openUrl(QUrl::fromLocalFile(m_controller->outputDirectory() + QDir::separator()));
}
void FileCollectDialog::cancel() { if (running()) m_controller->stop(); }
void FileCollectDialog::close()
{
	if (m_closed) return;
	m_closed = true;
	cancel();
	Q_EMIT finished();
}
