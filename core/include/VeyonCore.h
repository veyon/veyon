/*
 * VeyonCore.h - declaration of VeyonCore class + basic headers
 *
 * Copyright (c) 2006-2021 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QtEndian>
#include <QString>
#include <QDebug>

#include <atomic>
#include <functional>
#include <type_traits>

#include "QtCompat.h"

#if defined(BUILD_VEYON_CORE_LIBRARY)
#  define VEYON_CORE_EXPORT Q_DECL_EXPORT
#else
#  define VEYON_CORE_EXPORT Q_DECL_IMPORT
#endif

class QCoreApplication;
class QWidget;

class AuthenticationCredentials;
class BuiltinFeatures;
class CryptoCore;
class Filesystem;
class Logger;
class NetworkObjectDirectoryManager;
class PlatformPluginInterface;
class PlatformPluginManager;
class PluginManager;
class UserGroupsBackendManager;
class VeyonConfiguration;

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT VeyonCore : public QObject
{
	Q_OBJECT
public:
	enum class ApplicationVersion {
		Version_4_0,
		Version_4_1,
		Version_4_2,
		Version_4_3,
		Version_4_4,
		Version_4_5,
	};
	Q_ENUM(ApplicationVersion)

	enum class Component {
		Service,
		Server,
		Worker,
		Master,
		CLI,
		Configurator
	};
	Q_ENUM(Component)

	VeyonCore( QCoreApplication* application, Component component, const QString& appComponentName );
	~VeyonCore() override;

	static VeyonCore* instance();

	static QString version();
	static QString pluginDir();
	static QString translationsDirectory();
	static QString qtTranslationsDirectory();
	static QString executableSuffix();
	static QString sharedLibrarySuffix();

	static QString sessionIdEnvironmentVariable();

	static Component component()
	{
		return instance()->m_component;
	}

	static VeyonConfiguration& config()
	{
		return *( instance()->m_config );
	}

	static AuthenticationCredentials& authenticationCredentials()
	{
		return *( instance()->m_authenticationCredentials );
	}

	static CryptoCore& cryptoCore()
	{
		return *( instance()->m_cryptoCore );
	}

	static PluginManager& pluginManager()
	{
		return *( instance()->m_pluginManager );
	}

	static PlatformPluginInterface& platform()
	{
		return *( instance()->m_platformPlugin );
	}

	static BuiltinFeatures& builtinFeatures()
	{
		return *( instance()->m_builtinFeatures );
	}

	static UserGroupsBackendManager& userGroupsBackendManager()
	{
		return *( instance()->m_userGroupsBackendManager );
	}

	static NetworkObjectDirectoryManager& networkObjectDirectoryManager()
	{
		return *( instance()->m_networkObjectDirectoryManager );
	}

	static Filesystem& filesystem()
	{
		return *( instance()->m_filesystem );
	}

	static void setupApplicationParameters();
	bool initAuthentication();

	static int sessionId()
	{
		return instance()->m_sessionId;
	}

	static QString applicationName();
	static void enforceBranding( QWidget* topLevelWidget );

	static bool isDebugging();

	static QByteArray shortenFuncinfo( QByteArray info );
	static QByteArray cleanupFuncinfo( QByteArray info );

	static QString stripDomain( const QString& username );
	static QString formattedUuid( QUuid uuid );

	static bool isAuthenticationKeyNameValid( const QString& authKeyName );

	enum class AuthenticationMethod
	{
		LogonAuthentication,
		KeyFileAuthentication,
	};
	Q_ENUM(AuthenticationMethod)

	int exec();

private:
	void initPlatformPlugin();
	void initSession();
	void initConfiguration();
	void initLogging( const QString& appComponentName );
	void initLocaleAndTranslation();
	void initCryptoCore();
	void initAuthenticationCredentials();
	void initPlugins();
	void initManagers();
	void initLocalComputerControlInterface();
	bool initLogonAuthentication();
	bool initKeyFileAuthentication();
	void initSystemInfo();

	static VeyonCore* s_instance;

	Filesystem* m_filesystem;
	VeyonConfiguration* m_config;
	Logger* m_logger;
	AuthenticationCredentials* m_authenticationCredentials;
	CryptoCore* m_cryptoCore;
	PluginManager* m_pluginManager;
	PlatformPluginManager* m_platformPluginManager;
	PlatformPluginInterface* m_platformPlugin;
	BuiltinFeatures* m_builtinFeatures;
	UserGroupsBackendManager* m_userGroupsBackendManager;
	NetworkObjectDirectoryManager* m_networkObjectDirectoryManager;

	Component m_component;
	QString m_applicationName;
	bool m_debugging;

	int m_sessionId{0};

Q_SIGNALS:
	void initialized();
	void applicationLoaded();

};

#define V_FUNC_INFO VeyonCore::shortenFuncinfo(Q_FUNC_INFO).constData()

#define vDebug() if( VeyonCore::isDebugging()==false ); else qDebug() << V_FUNC_INFO
#define vInfo() qInfo() << V_FUNC_INFO
#define vWarning() qWarning() << V_FUNC_INFO
#define vCritical() qCritical() << V_FUNC_INFO
