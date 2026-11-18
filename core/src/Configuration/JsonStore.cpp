// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "Configuration/JsonStore.h"
#include "Configuration/Object.h"
#include "Filesystem.h"
#include "PlatformFilesystemFunctions.h"
#include "VeyonConfiguration.h"


namespace Configuration {

JsonStore::JsonStore(Scope scope, const QString& file) :
	Store(Store::Backend::JsonFile, scope),
	m_file(file)
{
}



static bool loadJsonTree(Object* obj, const QJsonObject& jsonParent, const QString& parentKey, QString* error)
{
	for (auto it = jsonParent.begin(); it != jsonParent.end(); ++it)
	{
		if (it.value().isObject())
		{
			QJsonObject jsonObject = it.value().toObject();

			if (jsonObject.contains(QStringLiteral("JsonStoreArray")))
			{
				if (jsonObject.size() != 1 || !jsonObject[QStringLiteral("JsonStoreArray")].isArray())
				{
					if (error) *error = QObject::tr("Invalid array value for %1.").arg(it.key());
					return false;
				}
				obj->setValue( it.key(), jsonObject[QStringLiteral("JsonStoreArray")].toArray(), parentKey );
			}
			else if (jsonObject.contains(QStringLiteral("JsonStoreObject")))
			{
				if (jsonObject.size() != 1 || !jsonObject[QStringLiteral("JsonStoreObject")].isObject())
				{
					if (error) *error = QObject::tr("Invalid object value for %1.").arg(it.key());
					return false;
				}
				obj->setValue( it.key(), jsonObject[QStringLiteral("JsonStoreObject")].toObject(), parentKey );
			}
			else
			{
				const QString subParentKey = parentKey + ( parentKey.isEmpty() ? QString() : QStringLiteral("/") ) + it.key();
				if (!loadJsonTree(obj, it.value().toObject(), subParentKey, error)) return false;
			}
		}
		else
		{
			obj->setValue(it.key(), it.value().toVariant(), parentKey);
		}
	}
	return true;
}



void JsonStore::load(Object* obj)
{
	if (!QFileInfo::exists(configurationFilePath())) return;
	QString error;
	if (!loadChecked(obj, &error)) vWarning() << error;
}

bool JsonStore::loadChecked(Object* object, QString* error)
{
	if (error) error->clear();
	QFile file(configurationFilePath());
	if (!file.open(QIODevice::ReadOnly))
	{
		if (error) *error = QObject::tr("Could not read %1: %2").arg(file.fileName(), file.errorString());
		return false;
	}
	const auto data = file.readAll();
	if (file.error() != QFileDevice::NoError)
	{
		if (error) *error = QObject::tr("Could not read %1: %2").arg(file.fileName(), file.errorString());
		return false;
	}
	QJsonParseError parseError;
	const auto document = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError)
	{
		if (error) *error = QObject::tr("Invalid JSON in %1 at byte %2: %3")
			.arg(file.fileName()).arg(parseError.offset).arg(parseError.errorString());
		return false;
	}
	if (!document.isObject())
	{
		if (error) *error = QObject::tr("The configuration in %1 must be a JSON object.").arg(file.fileName());
		return false;
	}
	Object loaded;
	if (!loadJsonTree(&loaded, document.object(), {}, error)) return false;
	*object += loaded;
	return true;
}



static QJsonObject saveJsonTree(const Object::DataMap& dataMap)
{
	QJsonObject jsonData;

	for (auto it = dataMap.begin(); it != dataMap.end(); ++it)
	{
		if (it.value().userType() == QMetaType::QVariantMap)
		{
			jsonData[it.key()] = saveJsonTree(it.value().toMap());
		}
		else if (it.value().userType() == QMetaType::QJsonArray)
		{
			QJsonObject jsonObj;
			jsonObj[QStringLiteral("JsonStoreArray")] = it.value().toJsonArray();
			jsonData[it.key()] = jsonObj;
		}
		else if (it.value().userType() == QMetaType::QJsonObject)
		{
			QJsonObject jsonObj;
			jsonObj[QStringLiteral("JsonStoreObject")] = it.value().toJsonObject();
			jsonData[it.key()] = jsonObj;
		}
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		else if (it.value().metaType().flags().testFlag(QMetaType::IsEnumeration))
#else
		else if (QMetaType(it.value().userType()).flags().testFlag(QMetaType::IsEnumeration))
#endif
		{
			jsonData[it.key()] = QJsonValue(it.value().toInt());
		}
		else
		{
			jsonData[it.key()] = QJsonValue::fromVariant(it.value());
		}
	}

	return jsonData;
}



void JsonStore::flush(const Object* obj)
{
	QString error;
	if (!flushChecked(obj, &error)) vCritical() << error;
}

bool JsonStore::flushChecked(const Object* object, QString* error)
{
	if (error) error->clear();
	QSaveFile file(configurationFilePath());
	const auto data = QJsonDocument(saveJsonTree(object->data())).toJson();
	if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
	{
		if (error) *error = QObject::tr("Could not write %1: %2").arg(file.fileName(), file.errorString());
		return false;
	}
	return true;
}



bool JsonStore::isWritable() const
{
	const QFileInfo file(configurationFilePath());
	return file.exists() ? file.isFile() && file.isWritable() : QFileInfo(file.absolutePath()).isWritable();
}



void JsonStore::clear()
{
	QString error;
	if (!clearChecked(&error)) vCritical() << error;
}

bool JsonStore::clearChecked(QString* error)
{
	const Object empty;
	return flushChecked(&empty, error);
}



QString JsonStore::configurationFilePath() const
{
	if (m_file.isEmpty() == false)
	{
		return m_file;
	}

	QString base;
	switch (scope())
	{
	case Scope::User:
		base = VeyonCore::config().userConfigurationDirectory();
		break;
	case Scope::System:
		base = VeyonCore::platform().filesystemFunctions().globalAppDataPath();
		break;
	}

	base = VeyonCore::filesystem().expandPath(base);

	VeyonCore::filesystem().ensurePathExists(base);

	auto fileNameBase = name();
	if (fileNameBase.isEmpty())
	{
		fileNameBase = configurationNameFromScope();
	}

	return QDir::toNativeSeparators(base + QDir::separator() + fileNameBase + QLatin1String(".json")); // clazy:exclude=qstring-allocations
}

}
