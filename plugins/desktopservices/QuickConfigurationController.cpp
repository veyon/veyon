// SPDX-License-Identifier: GPL-2.0-or-later
#include "QuickConfigurationController.h"
#include "DesktopServiceObject.h"
#include "ObjectManager.h"

DesktopServicesQuickConfigurationController::DesktopServicesQuickConfigurationController(DesktopServicesConfiguration* config, QObject* parent)
	: QObject(parent), m_configuration(config)
{
	config->setParent(this); setObjectName(QStringLiteral("presets"));
	connect(static_cast<Configuration::Object*>(config->object()), &Configuration::Object::configurationChanged, this, &DesktopServicesQuickConfigurationController::changed);
}
QVariantList DesktopServicesQuickConfigurationController::list(const QJsonArray& objects)
{
	QVariantList result;
	for (const auto& value : objects) { DesktopServiceObject object(value.toObject()); result.append(QVariantMap{{QStringLiteral("uid"), object.uid().toString()}, {QStringLiteral("name"), object.name()}, {QStringLiteral("path"), object.path()}}); }
	return result;
}
QVariantList DesktopServicesQuickConfigurationController::applications() const { return list(m_configuration->predefinedApplications()); }
QVariantList DesktopServicesQuickConfigurationController::websites() const { return list(m_configuration->predefinedWebsites()); }
bool DesktopServicesQuickConfigurationController::save(bool website, const QString& uid, const QString& name, const QString& path)
{
	if (name.trimmed().isEmpty() || path.trimmed().isEmpty()) return false;
	ObjectManager<DesktopServiceObject> manager(website ? m_configuration->predefinedWebsites() : m_configuration->predefinedApplications());
	DesktopServiceObject object(website ? DesktopServiceObject::Type::Website : DesktopServiceObject::Type::Application, name.trimmed(), path.trimmed(), QUuid(uid));
	if (uid.isEmpty()) manager.add(object); else manager.update(object);
	if (website) m_configuration->setPredefinedWebsites(manager.objects()); else m_configuration->setPredefinedApplications(manager.objects());
	return true;
}
void DesktopServicesQuickConfigurationController::remove(bool website, const QString& uid)
{
	ObjectManager<DesktopServiceObject> manager(website ? m_configuration->predefinedWebsites() : m_configuration->predefinedApplications()); manager.remove(QUuid(uid));
	if (website) m_configuration->setPredefinedWebsites(manager.objects()); else m_configuration->setPredefinedApplications(manager.objects());
}
void DesktopServicesQuickConfigurationController::move(bool website, const QString& uid, int delta)
{
	ObjectManager<DesktopServiceObject> manager(website ? m_configuration->predefinedWebsites() : m_configuration->predefinedApplications());
	if (delta < 0) manager.moveUp(QUuid(uid)); else manager.moveDown(QUuid(uid));
	if (website) m_configuration->setPredefinedWebsites(manager.objects()); else m_configuration->setPredefinedApplications(manager.objects());
}
