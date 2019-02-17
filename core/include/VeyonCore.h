/*
 * VeyonCore.h - declaration of VeyonCore class + basic headers
 *
 * Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef VEYON_CORE_H
#define VEYON_CORE_H

#include <QtEndian>
#include <QString>
#include <QDebug>

#include <atomic>
#include <functional>

#include "QtCompat.h"

#if defined(BUILD_VEYON_CORE_LIBRARY)
#  define VEYON_CORE_EXPORT Q_DECL_EXPORT
#else
#  define VEYON_CORE_EXPORT Q_DECL_IMPORT
#endif

class QCoreApplication;
class QWidget;

class AuthenticationCredentials;
class ComputerControlInterface;
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
	VeyonCore( QCoreApplication* application, const QString& appComponentName );
	~VeyonCore() override;

	static VeyonCore* instance();

	static QString version();
	static QString pluginDir();
	static QString translationsDirectory();
	static QString qtTranslationsDirectory();
	static QString executableSuffix();
	static QString sharedLibrarySuffix();

	static QString sessionIdEnvironmentVariable();

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

	static ComputerControlInterface& localComputerControlInterface()
	{
		return *( instance()->m_localComputerControlInterface );
	}

	static void setupApplicationParameters();
	bool initAuthentication( int credentialTypes );

	static bool hasSessionId();
	static int sessionId();

	static QString applicationName();
	static void enforceBranding( QWidget* topLevelWidget );

	static bool isDebugging();

	static QString stripDomain( const QString& username );
	static QString formattedUuid( QUuid uuid );

	const QString& authenticationKeyName() const
	{
		return m_authenticationKeyName;
	}

	static bool isAuthenticationKeyNameValid( const QString& authKeyName );

	typedef enum AuthenticationMethods
	{
		LogonAuthentication,
		KeyFileAuthentication,
		AuthenticationMethodCount
	} AuthenticationMethod;

private:
	void initPlatformPlugin();
	void initConfiguration();
	void initLogging( const QString& appComponentName );
	void initLocaleAndTranslation();
	void initCryptoCore();
	void initPlugins();
	void initManagers();
	void initLocalComputerControlInterface();
	bool initKeyFileAuthentication();

	static VeyonCore* s_instance;

	Filesystem* m_filesystem;
	VeyonConfiguration* m_config;
	Logger* m_logger;
	AuthenticationCredentials* m_authenticationCredentials;
	CryptoCore* m_cryptoCore;
	PluginManager* m_pluginManager;
	PlatformPluginManager* m_platformPluginManager;
	PlatformPluginInterface* m_platformPlugin;
	UserGroupsBackendManager* m_userGroupsBackendManager;
	NetworkObjectDirectoryManager* m_networkObjectDirectoryManager;

	ComputerControlInterface* m_localComputerControlInterface;

	QString m_applicationName;
	QString m_authenticationKeyName;

};

#endif
