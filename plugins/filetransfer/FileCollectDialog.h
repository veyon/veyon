// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QPointer>
#include <QVariantList>
class FileCollectController;
class QQmlApplicationEngine;
class QWindow;
class FileCollectDialog : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList collections READ collections NOTIFY changed)
	Q_PROPERTY(QString sourceDirectory READ sourceDirectory CONSTANT)
	Q_PROPERTY(QString filePattern READ filePattern CONSTANT)
	Q_PROPERTY(QString destinationDirectory READ destinationDirectory NOTIFY changed)
	Q_PROPERTY(int progress READ progress NOTIFY changed)
	Q_PROPERTY(bool running READ running NOTIFY changed)
	Q_PROPERTY(bool completed READ completed NOTIFY changed)
public:
	explicit FileCollectDialog(FileCollectController*, QWindow* parent = nullptr);
	~FileCollectDialog() override;
	void open();
	QVariantList collections() const;
	QString sourceDirectory() const;
	QString filePattern() const;
	QString destinationDirectory() const;
	int progress() const;
	bool running() const;
	bool completed() const { return m_completed; }
	Q_INVOKABLE void start(const QString& source, const QString& pattern, int subfolders);
	Q_INVOKABLE void browseDestination();
	Q_INVOKABLE void openOutputDirectory();
	Q_INVOKABLE void cancel();
	Q_INVOKABLE void close();
Q_SIGNALS:
	void changed();
	void finished();
private:
	QPointer<FileCollectController> m_controller;
	QPointer<QWindow> m_parentWindow;
	QQmlApplicationEngine* m_engine = nullptr;
	bool m_completed = false;
	bool m_closed = false;
};
