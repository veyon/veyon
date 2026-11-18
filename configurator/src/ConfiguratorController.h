// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ConfiguratorDraft.h"
#include "Configuration/PropertyModel.h"
#include <QVariantList>

class ConfiguratorController : public QObject
{
	Q_OBJECT
	Q_PROPERTY(Configuration::PropertyModel* properties READ properties CONSTANT)
	Q_PROPERTY(QVariantList pages READ pages CONSTANT)
	Q_PROPERTY(QVariantMap plugins READ plugins CONSTANT)
	Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
	Q_PROPERTY(bool applyPending READ applyPending NOTIFY changed)
	Q_PROPERTY(bool advanced READ advanced WRITE setAdvanced NOTIFY advancedChanged)
	Q_PROPERTY(QString error READ error NOTIFY changed)
	Q_PROPERTY(bool serviceRunning READ serviceRunning NOTIFY statusChanged)
	Q_PROPERTY(QStringList authorizedGroups READ authorizedGroups NOTIFY changed)
	Q_PROPERTY(int accessMode READ accessMode NOTIFY changed)
	Q_PROPERTY(QVariantList rules READ rules NOTIFY changed)
	Q_PROPERTY(QVariantList conditionTypes READ conditionTypes CONSTANT)
	Q_PROPERTY(QVariantList features READ features NOTIFY changed)
public:
	explicit ConfiguratorController(QObject* parent = nullptr);
	Configuration::PropertyModel* properties() { return m_properties; }
	QVariantList pages() const;
	QVariantMap plugins() const { return m_plugins; }
	bool dirty() const { return m_session.dirty(); }
	bool applyPending() const { return m_session.applyPending(); }
	bool advanced() const { return m_advanced; }
	void setAdvanced(bool value);
	QString error() const { return m_error; }
	bool serviceRunning() const;
	Q_INVOKABLE bool apply(bool forceSave = false);
	Q_INVOKABLE void revert();
	Q_INVOKABLE void factoryReset();
	Q_INVOKABLE bool importConfiguration(const QString& path);
	Q_INVOKABLE bool exportConfiguration(const QString& path);
	Q_INVOKABLE QString chooseFile(bool save, const QString& filter = QString());
	Q_INVOKABLE QString chooseDirectory();
	Q_INVOKABLE bool service(const QString& operation);
	Q_INVOKABLE void openLogDirectory();
	Q_INVOKABLE bool clearLogs();
	Q_INVOKABLE bool testAuthentication(const QString& username, const QString& password);
	QStringList authorizedGroups() const { return m_draft.authorizedUserGroups(); }
	int accessMode() const;
	Q_INVOKABLE void setAccessMode(int mode);
	Q_INVOKABLE QStringList availableGroups();
	Q_INVOKABLE QString testUserGroupAccess(const QString& username);
	Q_INVOKABLE QStringList ruleArguments(int condition);
	QVariantList rules() const;
	QVariantList conditionTypes() const;
	Q_INVOKABLE QVariantMap newRule() const;
	Q_INVOKABLE bool saveRule(int index, const QVariantMap& data);
	Q_INVOKABLE void removeRule(int index);
	Q_INVOKABLE void moveRule(int index, int delta);
	Q_INVOKABLE QString testRules(const QVariantMap& scenario);
	QVariantList features() const;
	Q_INVOKABLE void setFeatureEnabled(const QString& uid, bool enabled);
	Q_INVOKABLE void clearError() { m_error.clear(); Q_EMIT changed(); }
Q_SIGNALS:
	void changed();
	void statusChanged();
	void advancedChanged();
	void restartRecommended();
private:
	bool fail(const QString& message);
	ConfiguratorDraft m_session;
	VeyonConfiguration& m_draft{m_session.configuration()};
	Configuration::PropertyModel* m_properties;
	QVariantMap m_plugins;
	QString m_error;
	bool m_advanced = false;
};
