/*
 * VeyonCore.h - declaration of VeyonCore class + basic headers
 *
 * Copyright (c) 2006-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QObject>
#include <QtEndian>
#include <QVersionNumber>
#include <QDebug>

#include <array>
#include <atomic>
#include <functional>
#include <type_traits>

#include "QtCompat.h"

#if defined(veyon_core_EXPORTS)
#  define VEYON_CORE_EXPORT Q_DECL_EXPORT
#else
#  define VEYON_CORE_EXPORT Q_DECL_IMPORT
#endif

class QCoreApplication;
class QScreen;
class QSslConfiguration;
class QWidget;

class AuthenticationCredentials;
class AuthenticationManager;
class BuiltinFeatures;
class CryptoCore;
class FeatureManager;
class Filesystem;
class Logger;
class NetworkObjectDirectoryManager;
class PlatformPluginInterface;
class PlatformPluginManager;
class PluginManager;
class QmlCore;
class UserGroupsBackendManager;
class VeyonConfiguration;

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT VeyonCore : public QObject
{
	Q_OBJECT
public:
	using TlsConfiguration = QSslConfiguration;

	enum class ApplicationVersion {
		Unknown = -1,
		Version_4_0,
		Version_4_1,
		Version_4_2,
		Version_4_3,
		Version_4_4,
		Version_4_5,
		Version_4_6,
		Version_4_7,
		Version_4_8,
		Version_5_0,
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

	enum class UiStyle {
		Fusion,
		Native
	};
	Q_ENUM(UiStyle)

	static constexpr char RfbSecurityTypeVeyon = 40;

	VeyonCore( QCoreApplication* application, Component component, const QString& appComponentName );
	~VeyonCore() override;

	static VeyonCore* instance();

	static QString versionString();
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

	static AuthenticationManager& authenticationManager()
	{
		return *( instance()->m_authenticationManager );
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

	static QmlCore& qmlCore()
	{
		return *( instance()->m_qmlCore );
	}

	static BuiltinFeatures& builtinFeatures()
	{
		return *( instance()->m_builtinFeatures );
	}

	static FeatureManager& featureManager()
	{
		return *( instance()->m_featureManager );
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

	static int sessionId()
	{
		return instance()->m_sessionId;
	}

	static QString applicationName();
	static void enforceBranding( QWidget* topLevelWidget );

	static bool isDebugging();

	static QByteArray shortenFuncinfo( const QByteArray& info );
	static QByteArray cleanupFuncinfo( QByteArray info );

	static QString stripDomain( const QString& username );
	static QString formattedUuid( QUuid uuid );
	static QString stringify( const QVariantMap& map );

	static QString screenName( const QScreen& screen, int index );

	int exec();

private:
	void initPlatformPlugin();
	void initSession();
	void initConfiguration();
	void initLogging( const QString& appComponentName );
	void initLocaleAndTranslation();
	void initUi();
	void initCryptoCore();
	void initQmlCore();
	void initAuthenticationCredentials();
	void initPlugins();
	void initManagers();
	void initLocalComputerControlInterface();
	void initSystemInfo();
	void initTlsConfiguration();

	bool loadCertificateAuthorityFiles( TlsConfiguration* tlsConfig );
	bool addSelfSignedHostCertificate( TlsConfiguration* tlsConfig );

	static VeyonCore* s_instance;

	Filesystem* m_filesystem;
	VeyonConfiguration* m_config;
	Logger* m_logger;
	AuthenticationCredentials* m_authenticationCredentials;
	AuthenticationManager* m_authenticationManager;
	CryptoCore* m_cryptoCore;
	QmlCore* m_qmlCore{nullptr};
	PluginManager* m_pluginManager;
	PlatformPluginManager* m_platformPluginManager;
	PlatformPluginInterface* m_platformPlugin;
	BuiltinFeatures* m_builtinFeatures;
	FeatureManager* m_featureManager{nullptr};
	UserGroupsBackendManager* m_userGroupsBackendManager;
	NetworkObjectDirectoryManager* m_networkObjectDirectoryManager;

	Component m_component;
	QString m_applicationName;
	bool m_debugging;

	int m_sessionId{0};

Q_SIGNALS:
	void initialized();
	void applicationLoaded();
	void exited();

};

#define V_FUNC_INFO VeyonCore::shortenFuncinfo(Q_FUNC_INFO).constData()

#define vDebug() if( VeyonCore::isDebugging()==false ); else qDebug() << V_FUNC_INFO
#define vInfo() qInfo() << V_FUNC_INFO
#define vWarning() qWarning() << V_FUNC_INFO
#define vCritical() qCritical() << V_FUNC_INFO
