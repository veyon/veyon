// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <memory>

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
	QString error;
	if (!loadChecked(obj, &error)) vWarning() << error;
}

static bool checkSettingsStatus(QSettings* settings, QString* error)
{
	settings->sync();
	if (error) error->clear();
	if (settings->status() == QSettings::NoError) return true;
	if (error)
	{
		*error = settings->status() == QSettings::AccessError ?
			QObject::tr("Could not access configuration store %1. Check permissions and available disk space.").arg(settings->fileName()) :
			QObject::tr("The configuration store %1 has an invalid format.").arg(settings->fileName());
	}
	return false;
}

bool LocalStore::loadChecked(Object* object, QString* error)
{
	std::unique_ptr<QSettings> settings(createSettingsObject());
	if (!checkSettingsStatus(settings.get(), error)) return false;
	Object loaded;
	loadSettingsTree(&loaded, settings.get(), {});
	if (!checkSettingsStatus(settings.get(), error)) return false;
	*object += loaded;
	return true;
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
	QString error;
	if (!flushChecked(obj, &error)) vCritical() << error;
}

bool LocalStore::flushChecked(const Object* object, QString* error)
{
	std::unique_ptr<QSettings> settings(createSettingsObject());
	settings->setFallbacksEnabled(false);
	if (!checkSettingsStatus(settings.get(), error)) return false;
	if (!settings->isWritable())
	{
		if (error) *error = QObject::tr("Configuration is not writable. Please check your permissions!");
		return false;
	}
	settings->clear();
	saveSettingsTree(object->data(), settings.get());
	return checkSettingsStatus(settings.get(), error);
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
	QString error;
	if (!clearChecked(&error)) vCritical() << error;
}

bool LocalStore::clearChecked(QString* error)
{
	const Object empty;
	return flushChecked(&empty, error);
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
