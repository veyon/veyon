// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>

#include "Configuration/LocalStore.h"
#include "Configuration/Object.h"


namespace Configuration {

LocalStore::LocalStore(Scope scope) :
	Store(Store::Backend::Local, scope)
{
}



static void loadSettingsTree(Object* obj, QSettings* s,
							 const QString& parentKey)
{
	const auto childGroups = s->childGroups();

	for (const auto& g : childGroups)
	{
		const QString subParentKey = parentKey +
			(parentKey.isEmpty() ? QString() : QStringLiteral("/")) + g;
		s->beginGroup(g);
		loadSettingsTree(obj, s, subParentKey);
		s->endGroup();
	}

	const auto childKeys = s->childKeys();

	for (const auto& k : childKeys)
	{
		const auto stringValue = s->value(k).toString();
		static const QRegularExpression jsonValueRX(QStringLiteral("^@JsonValue(\\(.*\\))$"));
		const auto jsonValueMatch = jsonValueRX.match(stringValue);

		if (jsonValueMatch.hasMatch())
		{
			auto jsonValue = QJsonDocument::fromJson(QByteArray::fromBase64(jsonValueMatch.captured(1).toUtf8())).object();
			if (jsonValue.contains(QStringLiteral("a")))
			{
				obj->setValue(k, jsonValue[QStringLiteral("a")].toArray(), parentKey);
			}
			else if (jsonValue.contains(QStringLiteral("o")))
			{
				obj->setValue(k, jsonValue[QStringLiteral("o")].toObject(), parentKey);
			}
			else
			{
				vCritical() << "trying to load unknown JSON value type!";
			}
		}
		else
		{
			obj->setValue(k, s->value(k), parentKey);
		}
	}
}



void LocalStore::load(Object* obj)
{
	auto s = createSettingsObject();
	loadSettingsTree(obj, s, {});
	delete s;
}



static QString serializeJsonValue(const QJsonValue& jsonValue)
{
	QJsonObject jsonObject;

	if (jsonValue.isArray())
	{
		jsonObject[QStringLiteral("a")] = jsonValue;
	}
	else if (jsonValue.isObject())
	{
		jsonObject[QStringLiteral("o")] = jsonValue;
	}
	else
	{
		vCritical() << "trying to save unknown JSON value type" << jsonValue.type();
	}

	return QStringLiteral("@JsonValue(%1)").arg(QString::fromLatin1(QJsonDocument(jsonObject).toJson(QJsonDocument::Compact).toBase64()));
}



static void saveSettingsTree(const Object::DataMap& dataMap, QSettings* s)
{
	for (auto it = dataMap.begin(); it != dataMap.end(); ++it)
	{
		if (it.value().userType() == QMetaType::QVariantMap)
		{
			s->beginGroup(it.key());
			saveSettingsTree(it.value().toMap(), s);
			s->endGroup();
		}
		else if (it.value().userType() == QMetaType::QJsonArray)
		{
			s->setValue(it.key(), serializeJsonValue(it.value().toJsonArray()));
		}
		else if (it.value().userType() == QMetaType::QJsonObject)
		{
			s->setValue(it.key(), serializeJsonValue(it.value().toJsonObject()));
		}
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		else if (it.value().metaType().flags().testFlag(QMetaType::IsEnumeration))
#else
		else if (QMetaType(it.value().userType()).flags().testFlag(QMetaType::IsEnumeration))
#endif
		{
			s->setValue(it.key(), it.value().toInt());
		}
		else
		{
			s->setValue(it.key(), it.value());
		}
	}
}



void LocalStore::flush(const Object* obj)
{
	auto s = createSettingsObject();
	// clear previously saved items
	s->setFallbacksEnabled(false);
	s->clear();
	saveSettingsTree(obj->data(), s);
	delete s;
}



bool LocalStore::isWritable() const
{
	auto s = createSettingsObject();
	bool ret = s->isWritable();
	delete s;

	return ret;
}



void LocalStore::clear()
{
	auto s = createSettingsObject();
	s->setFallbacksEnabled(false);
	s->clear();
	delete s;
}



QSettings* LocalStore::createSettingsObject() const
{
	return new QSettings(
#ifdef Q_OS_WIN
		QSettings::Registry64Format,
#else
		QSettings::NativeFormat,
#endif
		scope() == Scope::System ? QSettings::SystemScope : QSettings::UserScope,
		QCoreApplication::organizationName(),
		QCoreApplication::applicationName());
}


}
