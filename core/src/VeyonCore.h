// SPDX-FileCopyrightText: 2006-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDebug>
#include <QFlags>
#include <QObject>
#include <QtEndian>

#include <atomic>
#include <functional>
#include <type_traits>

#if defined(veyon_core_EXPORTS)
#define VEYON_CORE_EXPORT Q_DECL_EXPORT
#else
#define VEYON_CORE_EXPORT Q_DECL_IMPORT
#endif

class QCoreApplication;
class QScreen;
class QWidget;

class AuthenticationCredentials;
class BuiltinFeatures;
class CryptoCore;
class FeatureManager;
class Filesystem;
class IpcClient;
class IpcEndpointCredentials;
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
		Version_4_9,
		Version_4_10,
		Version_4_11,
	};
	Q_ENUM(ApplicationVersion)

	enum class Component {
		Service,
		Server,
		Worker,
		Master,
		CLI,
		Configurator,
		AuthHelper,
		// veyon-capture-helper (02-ipc.md §1a.5): dumpable, so it initializes with
		// minimalInitialization() only (no ServiceIpc, no SecretStore)
		CaptureHelper,
	};
	Q_ENUM(Component)

	enum class UiStyle {
		Fusion,
		Native
	};
	Q_ENUM(UiStyle)

	enum class UiColorScheme {
		System,
		Light,
		Dark
	};
	Q_ENUM(UiColorScheme)

	// what a component initializes and may use; each component passes the flags it needs, the
	// component itself is identity only (logging, session ID, paths)
	enum class InitializationFlag {
		Ui = 0x0001,
		Translations = 0x0002,
		LegacyCrypto = 0x0004, // legacy QCA-based crypto wrapper (cryptoCore())
		AuthenticationCredentials = 0x0008,
		Plugins = 0x0010, // all non-platform plugins (the platform plugin is always loaded)
		Managers = 0x0020,
		Features = 0x0040,
		WritableConfiguration = 0x0080, // configuration upgrade and generation of the installation ID
		// capabilities, never part of a preset
		ServiceIpc = 0x0100,  // may connect to veyon-service via connectToServiceIpc() (02-ipc.md §6)
		SecretStore = 0x0200, // may use the SecretStore (03-secretstore-config.md §2)
	};
	Q_DECLARE_FLAGS(InitializationFlags, InitializationFlag)

	// functions instead of static constexpr members: a static data member of an exported class is
	// imported from the DLL on Windows when it is ODR-used (e.g. bound to operator|()'s reference
	// parameter), and the DLL does not export it
	static constexpr InitializationFlags defaultInitialization()
	{
		return InitializationFlags(InitializationFlag::Ui) | InitializationFlag::Translations |
			InitializationFlag::LegacyCrypto | InitializationFlag::AuthenticationCredentials |
			InitializationFlag::Plugins | InitializationFlag::Managers | InitializationFlag::Features |
			InitializationFlag::WritableConfiguration;
	}
	// platform plugin, read-only configuration, logger and session ID only
	static constexpr InitializationFlags minimalInitialization()
	{
		return {};
	}

	VeyonCore(QCoreApplication* application, Component component, const QString& appComponentName,
			  InitializationFlags flags);
	~VeyonCore() override;

	static VeyonCore* instance();

	static QString versionString();
	static QString pluginDir();
	static QString translationsDirectory();
	static QString qtTranslationsDirectory();
	static QString executableSuffix();
	static QString sharedLibrarySuffix();
	static QString applicationsDirectory();

	static QString sessionIdEnvironmentVariable();

	static Component component()
	{
		return instance()->m_component;
	}

	static InitializationFlags initializationFlags()
	{
		return instance()->m_initializationFlags;
	}

	static bool hasInitializationFlag(InitializationFlag flag)
	{
		return instance()->m_initializationFlags.testFlag(flag);
	}

	static VeyonConfiguration& config()
	{
		return *(instance()->m_config);
	}

	static AuthenticationCredentials& authenticationCredentials()
	{
		return *(instance()->m_authenticationCredentials);
	}

	static CryptoCore& cryptoCore()
	{
		return *(instance()->m_cryptoCore);
	}

	static PluginManager& pluginManager()
	{
		return *(instance()->m_pluginManager);
	}

	static PlatformPluginInterface& platform()
	{
		return *(instance()->m_platformPlugin);
	}

	static BuiltinFeatures& builtinFeatures()
	{
		return *(instance()->m_builtinFeatures);
	}

	static FeatureManager& featureManager()
	{
		return *(instance()->m_featureManager);
	}

	static UserGroupsBackendManager& userGroupsBackendManager()
	{
		return *(instance()->m_userGroupsBackendManager);
	}

	static NetworkObjectDirectoryManager& networkObjectDirectoryManager()
	{
		return *(instance()->m_networkObjectDirectoryManager);
	}

	static Filesystem& filesystem()
	{
		return *(instance()->m_filesystem);
	}

	// connection of a session server to veyon-service (02-ipc.md §6), nullptr if not connected
	static IpcClient* serviceIpcClient()
	{
		return instance()->m_serviceIpcClient;
	}

	[[nodiscard]] bool connectToServiceIpc(IpcEndpointCredentials&& credentials);

	static void setupApplicationParameters();
	bool initAuthentication();

	static int sessionId()
	{
		return instance()->m_sessionId;
	}

	static bool useDarkMode();

	static bool isDebugging();

	static QByteArray shortenFuncinfo(const QByteArray& info);
	static QByteArray cleanupFuncinfo(QByteArray info);

	static QString stripDomain(const QString& username);
	static QString stringify(const QVariantMap& map);

	static QString screenName(const QScreen& screen, int index);

	static bool isAuthenticationKeyNameValid(const QString& authKeyName);

	enum class AuthenticationMethod {
		LogonAuthentication,
		KeyFileAuthentication,
	};
	Q_ENUM(AuthenticationMethod)

	// TLS/ServerMode, TLS/ClientMode, TLS/IpTargetPolicy (03-secretstore-config.md §5, 05-tls.md);
	// the enumerator names are the configuration and trust bundle values
	enum class TlsServerMode {
		Disabled,
		Optional,
		Required,
	};
	Q_ENUM(TlsServerMode)

	enum class TlsClientMode {
		Disabled,
		Prefer,
		Required,
	};
	Q_ENUM(TlsClientMode)

	enum class TlsIpTargetPolicy {
		RequireIpSan,
		AcceptCaSignedWithHostname,
		AcceptCaSignedWithReverseDnsHostname,
	};
	Q_ENUM(TlsIpTargetPolicy)

	int exec();

private:
	void initPlatformPlugin();
	void initSession();
	void initConfiguration();
	void initLogging(const QString& appComponentName);
	void initLocaleAndTranslation();
	void initUi();
	void initCryptoCore();
	void initAuthenticationCredentials();
	void initPlugins();
	void initManagers();
	void initFeatures();
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
	FeatureManager* m_featureManager{nullptr};
	UserGroupsBackendManager* m_userGroupsBackendManager;
	NetworkObjectDirectoryManager* m_networkObjectDirectoryManager;
	IpcClient* m_serviceIpcClient{nullptr};

	Component m_component;
	InitializationFlags m_initializationFlags;
	bool m_debugging;

	int m_sessionId{0};

Q_SIGNALS:
	void initialized();
	void applicationLoaded();
	void exited();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VeyonCore::InitializationFlags)

#define V_FUNC_INFO VeyonCore::shortenFuncinfo(Q_FUNC_INFO).constData()

#define vDebug() \
	if (VeyonCore::isDebugging() == false) \
		; \
	else \
		qDebug() << V_FUNC_INFO
#define vInfo() qInfo() << V_FUNC_INFO
#define vWarning() qWarning() << V_FUNC_INFO
#define vCritical() qCritical() << V_FUNC_INFO
