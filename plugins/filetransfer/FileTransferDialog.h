// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QPointer>
#include <QVariantList>
#include "ComputerControlInterface.h"
class QQmlApplicationEngine;
class QWindow;
class FileTransferController;
class FileTransferDialog : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList files READ files NOTIFY changed)
	Q_PROPERTY(QVariantList targets READ targets NOTIFY changed)
	Q_PROPERTY(int progress READ progress NOTIFY changed)
	Q_PROPERTY(bool running READ running NOTIFY changed)
	Q_PROPERTY(bool completed READ completed NOTIFY changed)
	Q_PROPERTY(QString errors READ errors NOTIFY changed)
public:
	FileTransferDialog(FileTransferController*, QWindow*, const ComputerControlInterfaceList&);
	~FileTransferDialog() override;
	void open();
	QVariantList files() const;
	QVariantList targets() const;
	int progress() const { return m_progress; }
	bool running() const;
	bool completed() const { return m_completed; }
	QString errors() const { return m_errors; }
	Q_INVOKABLE void start(const QString& destination, int openMode, bool overwrite);
	Q_INVOKABLE void cancel();
	Q_INVOKABLE void close();
Q_SIGNALS:
	void changed();
	void finished();
private:
	QPointer<FileTransferController> m_controller;
	ComputerControlInterfaceList m_interfaces;
	QPointer<QWindow> m_parentWindow;
	QQmlApplicationEngine* m_engine = nullptr;
	int m_progress = 0;
	bool m_completed = false;
	bool m_closed = false;
	bool m_cancelled = false;
	QString m_errors;
};
