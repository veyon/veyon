// SPDX-License-Identifier: GPL-2.0-or-later
#include "QuickConfigurationController.h"
#include "NetworkObject.h"
#include "ObjectManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>

BuiltinDirectoryQuickConfigurationController::BuiltinDirectoryQuickConfigurationController(BuiltinDirectoryConfiguration* config, QObject* parent)
	: QObject(parent), m_configuration(config)
{
	config->setParent(this); setObjectName(QStringLiteral("directory"));
	connect(static_cast<Configuration::Object*>(config->object()), &Configuration::Object::configurationChanged, this, &BuiltinDirectoryQuickConfigurationController::changed);
}
QVariantList BuiltinDirectoryQuickConfigurationController::objects() const
{
	QVariantList result;
	for (const auto& value : m_configuration->networkObjects())
	{
		NetworkObject object(value.toObject());
		result.append(QVariantMap{{QStringLiteral("uid"), object.uid().toString()}, {QStringLiteral("parentUid"), object.parentUid().isNull() ? QString() : object.parentUid().toString()}, {QStringLiteral("type"), int(object.type())}, {QStringLiteral("name"), object.name()}, {QStringLiteral("host"), object.hostAddress()}, {QStringLiteral("mac"), object.macAddress()}, {QStringLiteral("directoryAddress"), object.directoryAddress()}});
	}
	return result;
}
bool BuiltinDirectoryQuickConfigurationController::save(const QVariantMap& values)
{
	if (values.value(QStringLiteral("name")).toString().trimmed().isEmpty()) return fail(tr("Enter a name."));
	auto uid = QUuid(values.value(QStringLiteral("uid")).toString()); if (uid.isNull()) uid = QUuid::createUuid();
	const auto parentUid = QUuid(values.value(QStringLiteral("parentUid")).toString());
	const auto type = static_cast<NetworkObject::Type>(values.value(QStringLiteral("type")).toInt());
	if (type != NetworkObject::Type::Location && type != NetworkObject::Type::Host) return fail(tr("Invalid object type."));
	auto array = m_configuration->networkObjects();
	if (!parentUid.isNull())
	{
		QMap<QUuid, NetworkObject> objects;
		for (const auto& value : array) { NetworkObject object(value.toObject()); objects.insert(object.uid(), object); }
		if (!objects.contains(parentUid) || !objects.value(parentUid).isContainer()) return fail(tr("Select a valid parent location."));
		QSet<QUuid> visited; auto ancestor = parentUid;
		while (!ancestor.isNull())
		{
			if (ancestor == uid || visited.contains(ancestor)) return fail(tr("A location cannot be placed inside itself."));
			visited.insert(ancestor); ancestor = objects.value(ancestor).parentUid();
		}
	}
	NetworkObject object(type, values.value(QStringLiteral("name")).toString().trimmed(), values.value(QStringLiteral("host")).toString().trimmed(), values.value(QStringLiteral("mac")).toString().trimmed(), values.value(QStringLiteral("directoryAddress")).toString(), uid, parentUid);
	ObjectManager<NetworkObject> manager(array);
	bool existing = false;
	for (const auto& value : array) if (NetworkObject(value.toObject()).uid() == uid) existing = true;
	if (existing) manager.update(object); else manager.add(object);
	m_error.clear(); m_configuration->setNetworkObjects(manager.objects()); return true;
}
bool BuiltinDirectoryQuickConfigurationController::remove(const QString& uid)
{
	ObjectManager<NetworkObject> manager(m_configuration->networkObjects());
	manager.remove(QUuid(uid), true); m_configuration->setNetworkObjects(manager.objects()); return true;
}
void BuiltinDirectoryQuickConfigurationController::move(const QString& uid, int delta)
{
	ObjectManager<NetworkObject> manager(m_configuration->networkObjects());
	if (delta < 0) manager.moveUp(QUuid(uid)); else manager.moveDown(QUuid(uid));
	m_configuration->setNetworkObjects(manager.objects());
}
bool BuiltinDirectoryQuickConfigurationController::importFile(const QString& path)
{
	QFile file(path); if (!file.open(QIODevice::ReadOnly)) return fail(file.errorString());
	QJsonParseError error; const auto document = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError || !document.isArray()) return fail(tr("Select a JSON array exported from the computer directory. %1").arg(error.errorString()));
	auto array = document.array(); QSet<QUuid> ids;
	for (const auto& value : array)
	{
		NetworkObject object(value.toObject());
		if (!value.isObject() || (object.type() != NetworkObject::Type::Location && object.type() != NetworkObject::Type::Host) || object.uid().isNull() || object.name().trimmed().isEmpty() || ids.contains(object.uid())) return fail(tr("The directory contains an invalid object or duplicate identifier."));
		ids.insert(object.uid());
	}
	for (const auto& value : array)
	{
		NetworkObject object(value.toObject());
		if (!object.parentUid().isNull() && !ids.contains(object.parentUid())) return fail(tr("The directory contains a missing parent location."));
		QSet<QUuid> visited; auto ancestor = object.uid();
		while (!ancestor.isNull())
		{
			if (visited.contains(ancestor)) return fail(tr("The directory contains a circular hierarchy."));
			visited.insert(ancestor);
			for (const auto& candidate : array) { NetworkObject parent(candidate.toObject()); if (parent.uid() == ancestor) { ancestor = parent.parentUid(); break; } }
		}
	}
	m_error.clear(); m_configuration->setNetworkObjects(array); return true;
}
bool BuiltinDirectoryQuickConfigurationController::exportFile(const QString& path)
{
	QSaveFile file(path); if (!file.open(QIODevice::WriteOnly)) return fail(file.errorString());
	const auto bytes = QJsonDocument(m_configuration->networkObjects()).toJson();
	if (file.write(bytes) != bytes.size() || !file.commit()) return fail(file.errorString());
	return true;
}
