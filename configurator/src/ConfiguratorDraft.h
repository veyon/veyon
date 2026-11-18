// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QBitArray>
#include "VeyonConfiguration.h"
#include "Configuration/JsonStore.h"

class ConfiguratorDraft
{
public:
	VeyonConfiguration& configuration() { return m_configuration; }
	const Configuration::Object::DataMap& baseline() const { return m_saved; }
	bool dirty() const { return m_applyPending || m_configuration.data() != m_saved; }
	bool applyPending() const { return m_applyPending; }
	bool restartPending() const { return m_restartPending; }
	bool requiresServiceRestart() const
	{
		Configuration::Object savedConfiguration;
		savedConfiguration.replaceData(m_saved);
		for (auto property : m_configuration.findChildren<Configuration::Property*>())
		{
			if (!property->uiMetadata().effects.testFlag(Configuration::Property::ApplyEffect::ServiceRestart)) continue;
			const auto absoluteKey = property->absoluteKey();
			const auto separator = absoluteKey.lastIndexOf(QLatin1Char('/'));
			const auto parentKey = separator < 0 ? QString() : absoluteKey.left(separator);
			const auto key = absoluteKey.mid(separator + 1);
			if (savedConfiguration.value(key, parentKey, property->defaultValue()) != property->variantValue())
				return true;
		}
		return false;
	}
	bool hasChangesInGroups(const QStringList& groups) const
	{
		auto keys = m_saved.keys();
		keys.append(m_configuration.data().keys());
		keys.removeDuplicates();
		QMap<QString, QVariant> defaults;
		for (auto property : m_configuration.findChildren<Configuration::Property*>())
			defaults.insert(property->absoluteKey(), property->defaultValue());
		for (const auto& key : keys)
		{
			if (groups.contains(key.section(QLatin1Char('/'), 0, 0)) &&
				m_saved.value(key, defaults.value(key)) != m_configuration.data().value(key, defaults.value(key)))
				return true;
		}
		return false;
	}

	void load(const Configuration::Object::DataMap& values)
	{
		m_configuration.replaceData(values);
		m_saved = values;
		m_applyPending = false;
		m_restartPending = false;
	}
	void revert() { m_configuration.replaceData(m_saved); }
	void factoryReset() { m_configuration.replaceData({}); }
	void saved(bool restartRequired)
	{
		m_saved = m_configuration.data();
		m_applyPending = true;
		m_restartPending |= restartRequired;
	}
	void applied()
	{
		m_applyPending = false;
		m_restartPending = false;
	}
	bool importConfiguration(const QString& path, QString* error)
	{
		Configuration::Object imported;
		imported.replaceData(m_configuration.data());
		Configuration::JsonStore store(Configuration::Store::Scope::System, path);
		if (!store.loadChecked(&imported, error)) return false;
		m_configuration.replaceData(imported.data());
		return true;
	}
	bool exportConfiguration(const QString& path, QString* error) const
	{
		Configuration::JsonStore store(Configuration::Store::Scope::System, path);
		return store.flushChecked(&m_configuration, error);
	}
private:
	VeyonConfiguration m_configuration{nullptr};
	Configuration::Object::DataMap m_saved;
	bool m_applyPending = false;
	bool m_restartPending = false;
};
