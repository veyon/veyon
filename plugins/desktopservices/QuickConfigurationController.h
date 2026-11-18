// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "DesktopServicesConfiguration.h"
#include <QVariantList>

class DesktopServicesQuickConfigurationController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList applications READ applications NOTIFY changed)
	Q_PROPERTY(QVariantList websites READ websites NOTIFY changed)
public:
	DesktopServicesQuickConfigurationController(DesktopServicesConfiguration* config, QObject* parent);
	QVariantList applications() const;
	QVariantList websites() const;
	Q_INVOKABLE bool save(bool website, const QString& uid, const QString& name, const QString& path);
	Q_INVOKABLE void remove(bool website, const QString& uid);
	Q_INVOKABLE void move(bool website, const QString& uid, int delta);
Q_SIGNALS:
	void changed();
private:
	static QVariantList list(const QJsonArray& objects);
	DesktopServicesConfiguration* m_configuration;
};
