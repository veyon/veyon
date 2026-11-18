// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later
#include "FileTransferDialog.h"
#include "FileTransferController.h"
#include "QuickUi.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>
#include <QFileInfo>

FileTransferDialog::FileTransferDialog(FileTransferController* controller, QWindow* parent,
	const ComputerControlInterfaceList& interfaces) :
	QObject(nullptr), m_controller(controller), m_interfaces(interfaces), m_parentWindow(parent)
{
	connect(controller, &FileTransferController::progressChanged, this, [this](int value) {
		m_progress = value;
		Q_EMIT changed();
	});
	connect(controller, &FileTransferController::started, this, [this]() {
		m_completed = false;
		m_cancelled = false;
		m_progress = 0;
		m_errors.clear();
		Q_EMIT changed();
	});
	connect(controller, &FileTransferController::finished, this, [this]() {
		m_completed = true;
		Q_EMIT changed();
	});
	connect(controller, &FileTransferController::filesChanged, this, &FileTransferDialog::changed);
	connect(controller, &FileTransferController::errorOccurred, this, [this](const QString& message) {
		if (!m_errors.isEmpty()) m_errors += QLatin1Char('\n');
		m_errors += message;
		Q_EMIT changed();
	});
	for (const auto& target : m_interfaces)
		connect(target.data(), &ComputerControlInterface::stateChanged, this, &FileTransferDialog::changed);
}
FileTransferDialog::~FileTransferDialog() { delete m_engine; }
void FileTransferDialog::open()
{
	m_engine = QuickUi::createEngine(this);
	m_engine->rootContext()->setContextProperty(QStringLiteral("transferDialog"), this);
	m_engine->load(QUrl(QStringLiteral("qrc:/filetransfer/qml/TransferWindow.qml")));
	if (m_engine->rootObjects().isEmpty()) {
		QuickUi::showError(tr("File transfer"), tr("The file transfer window could not be loaded."), m_parentWindow);
		Q_EMIT finished();
		return;
	}
	auto window = qobject_cast<QWindow*>(m_engine->rootObjects().first());
	if (window) window->setTransientParent(m_parentWindow);
}
bool FileTransferDialog::running() const { return m_controller && m_controller->isRunning(); }
QVariantList FileTransferDialog::files() const
{
	QVariantList rows;
	if (!m_controller) return rows;
	const auto current = m_controller->currentFileIndex();
	for (int i = 0; i < m_controller->files().size(); ++i)
		rows.append(QVariantMap{{QStringLiteral("name"), QFileInfo(m_controller->files().at(i)).fileName()},
			{QStringLiteral("path"), QFileInfo(m_controller->files().at(i)).absoluteFilePath()},
			{QStringLiteral("status"), i < current ? tr("Processed") :
				(i == current && running() ? tr("Sending") : tr("Queued"))}});
	return rows;
}
QVariantList FileTransferDialog::targets() const
{
	QVariantList rows;
	for (const auto& target : m_interfaces) {
		const bool connected = target->state() == ComputerControlInterface::State::Connected;
		rows.append(QVariantMap{{QStringLiteral("name"), target->computerName()},
			{QStringLiteral("status"), !connected ? tr("Disconnected") :
				(running() ? (target->isMessageQueueEmpty() ? tr("Ready") : tr("Sending")) :
				(m_completed ? (m_cancelled ? tr("Stopped") : tr("Processed")) : tr("Ready")))}});
	}
	return rows;
}
void FileTransferDialog::start(const QString& destination, int openMode, bool overwrite)
{
	if (!m_controller || running()) return;
	if (m_interfaces.isEmpty())
	{
		QuickUi::showError(tr("File transfer"), tr("No recipient computers are selected."), m_parentWindow);
		return;
	}
	FileTransferController::Flags flags(FileTransferController::Transfer);
	if (openMode == 1) flags |= FileTransferController::OpenTransferFolder;
	if (openMode == 2) flags |= FileTransferController::OpenFilesInApplication;
	if (overwrite) flags |= FileTransferController::OverwriteExistingFiles;
	m_controller->setDestinationDirectory(destination.trimmed());
	m_controller->setFlags(flags);
	m_controller->start();
}
void FileTransferDialog::cancel()
{
	if (running()) {
		m_cancelled = true;
		m_controller->stop();
	}
}
void FileTransferDialog::close()
{
	if (m_closed) return;
	m_closed = true;
	cancel();
	Q_EMIT finished();
}
