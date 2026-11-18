// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "VeyonConfiguration.h"
#include <QVariantList>

class AuthKeysQuickConfigurationController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList keys READ keys NOTIFY changed)
	Q_PROPERTY(QString message READ message NOTIFY changed)
public:
	AuthKeysQuickConfigurationController(VeyonConfiguration* draft, QObject* parent);
	QVariantList keys() const;
	QString message() const { return m_message; }
	Q_INVOKABLE bool create(const QString& name);
	Q_INVOKABLE bool remove(const QString& key);
	Q_INVOKABLE bool importKey(const QString& name, const QString& path);
	Q_INVOKABLE bool exportKey(const QString& key, const QString& path);
	Q_INVOKABLE bool setAccessGroup(const QString& key, const QString& group);
	Q_INVOKABLE bool test(const QString& key);
	Q_INVOKABLE void refresh() { Q_EMIT changed(); }
private:
	bool finish(bool success, const QString& message);
	VeyonConfiguration* m_draft;
	QString m_message;
Q_SIGNALS:
	void changed();
};
