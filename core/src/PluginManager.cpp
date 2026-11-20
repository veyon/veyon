// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDir>
#include <QMutex>
#include <QPluginLoader>

#include "Configuration/SecretPropertyRegistry.h"
#include "Logger.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"

#include <veyonconfig.h>

namespace {

// plugin files which failed to load, so that each one is logged only once per process
QMutex incompatiblePluginFilesMutex;
QStringList incompatiblePluginFilesList;

}

PluginManager::PluginManager(QObject* parent) :
	QObject(parent),
	m_pluginInterfaces(),
	m_pluginObjects(),
	m_pluginLoaders(),
	m_noDebugMessages(qEnvironmentVariableIsSet(Logger::logLevelEnvironmentVariable()))
{
	initPluginSearchPath();
}



PluginManager::~PluginManager()
{
	vDebug();

	for (auto pluginLoader : std::as_const(m_pluginLoaders))
	{
		pluginLoader->unload();
	}

	m_pluginInterfaces.clear();
	m_pluginObjects.clear();
}



void PluginManager::loadPlatformPlugins()
{
	loadPlugins(QStringLiteral("*-platform") + VeyonCore::sharedLibrarySuffix());
}



void PluginManager::loadPlugins()
{
	loadPlugins(QStringLiteral("*") + VeyonCore::sharedLibrarySuffix());

	// before anything uses the registry, e.g. the legacy secret migration (03-secretstore-config.md §3a)
	for (auto pluginInterface : std::as_const(m_pluginInterfaces))
	{
		Configuration::SecretPropertyRegistry::addPluginProperties(*pluginInterface);
	}
}



void PluginManager::upgradePlugins()
{
	auto versions = VeyonCore::config().pluginVersions();

	for (auto pluginInterface : std::as_const(m_pluginInterfaces))
	{
		const auto pluginUid = pluginInterface->uid().toString();
		auto previousPluginVersion = QVersionNumber::fromString(versions.value(pluginUid).toString());
		if (previousPluginVersion.isNull())
		{
			previousPluginVersion = QVersionNumber(1, 1);
		}
		const auto currentPluginVersion = pluginInterface->version();
		if (currentPluginVersion > previousPluginVersion)
		{
			vDebug() << "Upgrading plugin" << pluginInterface->name()
					 << "from" << previousPluginVersion.toString()
					 << "to" << currentPluginVersion.toString();
			pluginInterface->upgrade(previousPluginVersion);
		}

		versions[pluginUid] = currentPluginVersion.toString();
	}

	VeyonCore::config().setPluginVersions(versions);
}



void PluginManager::registerExtraPluginInterface(QObject* pluginObject)
{
	auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);
	if (pluginInterface)
	{
		m_pluginInterfaces += pluginInterface;
		m_pluginObjects += pluginObject;
	}
}



PluginUidList PluginManager::pluginUids() const
{
	PluginUidList pluginUidList;

	pluginUidList.reserve(m_pluginInterfaces.size());

	for (auto pluginInterface : std::as_const(m_pluginInterfaces))
	{
		pluginUidList += pluginInterface->uid();
	}

	return pluginUidList;
}



PluginInterface* PluginManager::pluginInterface(Plugin::Uid pluginUid)
{
	for (auto pluginInterface : std::as_const(m_pluginInterfaces))
	{
		if (pluginInterface->uid() == pluginUid)
		{
			return pluginInterface;
		}
	}

	return nullptr;
}



QString PluginManager::pluginName(Plugin::Uid pluginUid) const
{
	for (auto pluginInterface : m_pluginInterfaces)
	{
		if (pluginInterface->uid() == pluginUid)
		{
			return pluginInterface->name();
		}
	}

	return {};
}



QObject* PluginManager::loadPluginInstance(QPluginLoader& pluginLoader)
{
	// resolve all symbols at load time (RTLD_NOW on Linux), so that a plugin built against an
	// incompatible Veyon version fails to load instead of terminating the process with a symbol lookup
	// error as soon as a missing function is called
	pluginLoader.setLoadHints(pluginLoader.loadHints() | QLibrary::ResolveAllSymbolsHint);

	const auto pluginObject = pluginLoader.instance();
	if (pluginObject == nullptr && pluginLoader.metaData().isEmpty() == false)
	{
		const auto filePath = pluginLoader.fileName();

		QMutexLocker locker(&incompatiblePluginFilesMutex);
		if (incompatiblePluginFilesList.contains(filePath) == false)
		{
			incompatiblePluginFilesList.append(filePath);
			locker.unlock();

			vCritical() << "plugin" << filePath
						<< "is incompatible with this Veyon version and was not loaded:" << pluginLoader.errorString();
		}
	}

	return pluginObject;
}



QStringList PluginManager::incompatiblePluginFiles()
{
	QMutexLocker locker(&incompatiblePluginFilesMutex);
	return incompatiblePluginFilesList;
}



void PluginManager::initPluginSearchPath()
{
	QDir dir(QCoreApplication::applicationDirPath());
	if (dir.cd(VeyonCore::pluginDir()))
	{
		const auto pluginSearchPath = dir.absolutePath();
		if (m_noDebugMessages == false)
		{
			vDebug() << "Adding plugin search path" << pluginSearchPath;
		}
		m_pluginSearchPaths.append(pluginSearchPath);
		QCoreApplication::addLibraryPath(pluginSearchPath);
	}
#if defined(VEYON_WITH_TESTS)
	for (const auto& baseDir : {QStringLiteral(CMAKE_BINARY_DIR "/plugins/platform"),
								QStringLiteral(CMAKE_BINARY_DIR "/plugins/vncserver"),
								QStringLiteral(CMAKE_BINARY_DIR "/plugins/")})
	{
		const auto pluginDirs = QDir(baseDir).entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
		for (const auto& pluginDir : pluginDirs)
		{
			m_pluginSearchPaths.append(pluginDir.absoluteFilePath());
		}
	}
#endif
}



void PluginManager::loadPlugins(const QString& nameFilter)
{
	QFileInfoList plugins;
	for (const auto& pluginSearchPath : std::as_const(m_pluginSearchPaths))
	{
		plugins.append(QDir(pluginSearchPath).entryInfoList({nameFilter}));
	}

	for (const auto& fileInfo : plugins)
	{
		const auto fileName = fileInfo.fileName();

		// skip simple shared libraries
		if (fileName.startsWith(QLatin1String("lib")) &&
			fileName.startsWith(QLatin1String("libveyon")) == false)
		{
			continue;
		}

		auto pluginLoader = new QPluginLoader(fileInfo.filePath(), this);
		auto pluginObject = loadPluginInstance(*pluginLoader);
		auto pluginInterface = qobject_cast<PluginInterface*>(pluginObject);

		if (pluginObject && pluginInterface &&
			m_pluginInterfaces.contains(pluginInterface) == false)
		{
			if (m_noDebugMessages == false)
			{
				vDebug() << "discovered plugin" << pluginInterface->name() << "at" << fileInfo.filePath();
			}
			m_pluginInterfaces += pluginInterface; // clazy:exclude=reserve-candidates
			m_pluginObjects += pluginObject;	   // clazy:exclude=reserve-candidates
			m_pluginLoaders += pluginLoader;	   // clazy:exclude=reserve-candidates
		}
		else
		{
			if (pluginObject && pluginInterface == nullptr)
			{
				vCritical() << "plugin" << fileInfo.filePath()
							<< "does not implement the plugin interface of this Veyon version and was not loaded";
			}
			delete pluginLoader;
		}
	}
}
