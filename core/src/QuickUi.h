/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QObject>
#include <QVariantMap>
#include <QUrl>
#include "VeyonCore.h"

class QQmlApplicationEngine;
class QWindow;

class VEYON_CORE_EXPORT QuickUi : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool dark READ dark NOTIFY themeChanged)
public:
	explicit QuickUi(QObject* parent = nullptr);
	bool dark() const;
	static QQmlApplicationEngine* createEngine(QObject* parent = nullptr);
	static QVariantMap execDialog(const QString& kind, const QVariantMap& data = {}, QWindow* parent = nullptr);
	static void showError(const QString& title, const QString& text, QWindow* parent = nullptr);
	static void showNotification(const QString& title, const QString& text);
	Q_INVOKABLE void openUrl(const QUrl& url);
	Q_INVOKABLE QString localPath(const QUrl& url) const;
	Q_INVOKABLE QUrl fileUrl(const QString& path) const;
Q_SIGNALS:
	void themeChanged();
};

class VEYON_CORE_EXPORT QuickDialogRequest : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString kind READ kind CONSTANT)
	Q_PROPERTY(QVariantMap data READ data CONSTANT)
	Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
public:
	QuickDialogRequest(QString kind, QVariantMap data, QObject* parent);
	QString kind() const { return m_kind; }
	QVariantMap data() const { return m_data; }
	QString errorString() const { return m_error; }
	QVariantMap result() const { return m_result; }
	bool finished() const { return m_finished; }
	Q_INVOKABLE void accept(const QVariantMap& result = {});
	Q_INVOKABLE void reject();
Q_SIGNALS:
	void completed();
	void errorChanged();
private:
	QString m_kind;
	QVariantMap m_data;
	QVariantMap m_result;
	QString m_error;
	bool m_finished = false;
};
