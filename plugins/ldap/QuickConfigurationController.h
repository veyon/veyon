// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "LdapConfiguration.h"
#include <QThread>
#include <QVariantList>

class LdapQuickConfigurationController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int locationMode READ locationMode NOTIFY changed)
	Q_PROPERTY(QVariantList operations READ operations CONSTANT)
	Q_PROPERTY(bool busy READ busy NOTIFY changed)
	Q_PROPERTY(QStringList results READ results NOTIFY changed)
	Q_PROPERTY(QString error READ error NOTIFY changed)
	Q_PROPERTY(QString targetKey READ targetKey NOTIFY changed)
public:
	LdapQuickConfigurationController(LdapConfiguration* config, QObject* parent);
	~LdapQuickConfigurationController() override;
	QVariantList operations() const;
	int locationMode() const;
	Q_INVOKABLE void setLocationMode(int mode);
	bool busy() const { return m_thread != nullptr; }
	QStringList results() const { return m_results; }
	QString error() const { return m_error; }
	QString targetKey() const { return m_targetKey; }
	Q_INVOKABLE void run(const QString& operation, const QString& argument = QString());
	Q_INVOKABLE void browse(const QString& key, const QString& distinguishedName = QString(), bool attributes = false);
	Q_INVOKABLE bool select(const QString& value);
Q_SIGNALS:
	void changed();
private:
	void query(const QString& operation, const QString& argument, bool attributes = false);
	LdapConfiguration* m_configuration;
	QThread* m_thread = nullptr;
	QStringList m_results;
	QString m_error;
	QString m_targetKey;
};
