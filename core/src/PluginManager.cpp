// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QPluginLoader>

#include "Logger.h"
#include "PluginManager.h"
#include "VeyonConfiguration.h"

#include <veyonconfig.h>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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

	// load the library without instantiating the plugin, i.e. without running any of its code except
	// static initializers
	if (pluginLoader.load() == false)
	{
		// not reported for simple shared libraries
		if (pluginLoader.metaData().isEmpty() == false)
		{
			reportIncompatiblePlugin(pluginLoader.fileName(), pluginLoader.errorString());
		}
		return nullptr;
	}

	// there's no ABI compatibility between Veyon releases, so refuse plugins built for any other
	// Veyon version before their (potentially incompatible) vtables are used
	const auto pluginVersion = pluginBuildVersion(pluginLoader.fileName());
	if (pluginVersion != buildVersion())
	{
		const auto builtFor = pluginVersion.isEmpty() ? QStringLiteral("an unknown Veyon version") : QStringLiteral("Veyon %1").arg(pluginVersion);
		reportIncompatiblePlugin(pluginLoader.fileName(),
								 QStringLiteral("it was built for %1 but Veyon %2 requires plugins built for the "
												"same version")
									 .arg(builtFor, buildVersion()));
		pluginLoader.unload();
		return nullptr;
	}

	const auto pluginObject = pluginLoader.instance();
	if (pluginObject == nullptr)
	{
		reportIncompatiblePlugin(pluginLoader.fileName(), pluginLoader.errorString());
	}

	return pluginObject;
}



QString PluginManager::buildVersion()
{
	return QVersionNumber(VEYON_VERSION_MAJOR, VEYON_VERSION_MINOR, VEYON_VERSION_PATCH).toString();
}



QStringList PluginManager::incompatiblePluginFiles()
{
	QMutexLocker locker(&incompatiblePluginFilesMutex);
	return incompatiblePluginFilesList;
}



QString PluginManager::pluginBuildVersion(const QString& filePath)
{
	// resolve the symbol natively in the library loaded by QPluginLoader already, as a QLibrary for the
	// same file would share (and possibly unload) the plugin loader's library and plugin instance
	using BuildVersionFunction = const char* (*)();
	const auto symbol = "veyonPluginBuildVersion";

	QString version;

#if defined(Q_OS_WIN)
	const auto module = GetModuleHandleW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(filePath).utf16()));
	if (module)
	{
		const auto buildVersionFunction = reinterpret_cast<BuildVersionFunction>(GetProcAddress(module, symbol));
		if (buildVersionFunction)
		{
			version = QString::fromLatin1(buildVersionFunction());
		}
	}
#else
	const auto handle = dlopen(QFile::encodeName(filePath).constData(), RTLD_NOW | RTLD_NOLOAD);
	if (handle)
	{
		const auto buildVersionFunction = reinterpret_cast<BuildVersionFunction>(dlsym(handle, symbol));
		if (buildVersionFunction)
		{
			version = QString::fromLatin1(buildVersionFunction());
		}
		dlclose(handle);
	}
#endif

	return version;
}



void PluginManager::reportIncompatiblePlugin(const QString& filePath, const QString& reason)
{
	QMutexLocker locker(&incompatiblePluginFilesMutex);
	if (incompatiblePluginFilesList.contains(filePath) == false)
	{
		incompatiblePluginFilesList.append(filePath);
		locker.unlock();

		vCritical() << "plugin" << filePath << "is incompatible with this Veyon version and was not loaded:" << reason;
	}
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
			// loadPluginInstance() calls QPluginLoader::load() explicitly, which takes an unload reference
			// even if the library has been loaded by another QPluginLoader already (e.g. platform plugins
			// are loaded by both loadPlatformPlugins() and loadPlugins()). Deleting a QPluginLoader does not
			// release this reference, so balance it here. Otherwise the plugin instance is not deleted in
			// ~PluginManager() but only by QtCore's library cleanup at process exit, i.e. after the
			// application object has been destroyed, which crashes in plugins owning GUI objects such as
			// the dummy QWindow of the Windows platform plugin.
			pluginLoader->unload();
			delete pluginLoader;
		}
	}
}
