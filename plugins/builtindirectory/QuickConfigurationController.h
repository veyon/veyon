// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "BuiltinDirectoryConfiguration.h"
#include <QVariantList>

class BuiltinDirectoryQuickConfigurationController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList objects READ objects NOTIFY changed)
	Q_PROPERTY(QString error READ error NOTIFY changed)
public:
	BuiltinDirectoryQuickConfigurationController(BuiltinDirectoryConfiguration* config, QObject* parent);
	QVariantList objects() const;
	QString error() const { return m_error; }
	Q_INVOKABLE bool save(const QVariantMap& object);
	Q_INVOKABLE bool remove(const QString& uid);
	Q_INVOKABLE void move(const QString& uid, int delta);
	Q_INVOKABLE bool importFile(const QString& path);
	Q_INVOKABLE bool exportFile(const QString& path);
Q_SIGNALS:
	void changed();
private:
	bool fail(const QString& error) { m_error = error; Q_EMIT changed(); return false; }
	BuiltinDirectoryConfiguration* m_configuration;
	QString m_error;
};
