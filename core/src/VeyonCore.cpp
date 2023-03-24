/*
 * VeyonCore.cpp - implementation of Veyon Core
 *
 * Copyright (c) 2006-2022 Tobias Junghans <tobydox@veyon.io>
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

#include <veyonconfig.h>

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QGroupBox>
#include <QJsonDocument>
#include <QLabel>
#include <QLibraryInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSslConfiguration>
#include <QSslKey>
#include <QStyleFactory>
#include <QSysInfo>

#include "AuthenticationCredentials.h"
#include "AuthenticationManager.h"
#include "BuiltinFeatures.h"
#include "FeatureManager.h"
#include "Filesystem.h"
#include "HostAddress.h"
#include "Logger.h"
#include "NetworkObjectDirectoryManager.h"
#include "PlatformPluginManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformSessionFunctions.h"
#include "PluginManager.h"
#include "QmlCore.h"
#include "TranslationLoader.h"
#include "UserGroupsBackendManager.h"
#include "VeyonConfiguration.h"
#include "VncConnection.h"


VeyonCore* VeyonCore::s_instance = nullptr;


VeyonCore::VeyonCore( QCoreApplication* application, Component component, const QString& appComponentName ) :
	QObject( application ),
	m_filesystem( new Filesystem ),
	m_config( nullptr ),
	m_logger( nullptr ),
	m_authenticationCredentials( nullptr ),
	m_authenticationManager( nullptr ),
	m_cryptoCore( nullptr ),
	m_pluginManager( nullptr ),
	m_platformPluginManager( nullptr ),
	m_platformPlugin( nullptr ),
	m_builtinFeatures( nullptr ),
	m_userGroupsBackendManager( nullptr ),
	m_networkObjectDirectoryManager( nullptr ),
	m_component( component ),
	m_applicationName( QStringLiteral( "Veyon" ) ),
	m_debugging( false )
{
	Q_ASSERT( application != nullptr );

	s_instance = this;

	initPlatformPlugin();

	initConfiguration();

	initSession();

	initLogging( appComponentName );

	initLocaleAndTranslation();

	initUi();

	initCryptoCore();

	initTlsConfiguration();

	initQmlCore();

	initAuthenticationCredentials();

	initPlugins();

	initManagers();

	initSystemInfo();

	Q_EMIT initialized(); // clazy:exclude=incorrect-emit
}



VeyonCore::~VeyonCore()
{
	vDebug();

	delete m_featureManager;
	m_featureManager = nullptr;

	delete m_userGroupsBackendManager;
	m_userGroupsBackendManager = nullptr;

	delete m_authenticationCredentials;
	m_authenticationCredentials = nullptr;

	delete m_authenticationManager;
	m_authenticationManager = nullptr;

	delete m_builtinFeatures;
	m_builtinFeatures = nullptr;

	delete m_logger;
	m_logger = nullptr;

	delete m_platformPluginManager;
	m_platformPluginManager = nullptr;

	delete m_pluginManager;
	m_pluginManager = nullptr;

	delete m_config;
	m_config = nullptr;

	delete m_filesystem;
	m_filesystem = nullptr;

	delete m_cryptoCore;
	m_cryptoCore = nullptr;

	delete m_qmlCore;
	m_qmlCore = nullptr;

	s_instance = nullptr;
}



VeyonCore* VeyonCore::instance()
{
	return s_instance;
}



QString VeyonCore::versionString()
{
	return QStringLiteral( VEYON_VERSION );
}



QString VeyonCore::pluginDir()
{
	return QStringLiteral( VEYON_PLUGIN_DIR );
}



QString VeyonCore::translationsDirectory()
{
	return QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral(VEYON_TRANSLATIONS_DIR);
}



QString VeyonCore::qtTranslationsDirectory()
{
	const auto path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
	if( QDir{path}.exists() )
	{
		return path;
	}

	return translationsDirectory();
}



QString VeyonCore::executableSuffix()
{
	return QStringLiteral( VEYON_EXECUTABLE_SUFFIX ); // clazy:exclude=empty-qstringliteral
}



QString VeyonCore::sharedLibrarySuffix()
{
	return QStringLiteral( VEYON_SHARED_LIBRARY_SUFFIX );
}



QString VeyonCore::sessionIdEnvironmentVariable()
{
	return QStringLiteral("VEYON_SESSION_ID");
}



void VeyonCore::setupApplicationParameters()
{
	QCoreApplication::setOrganizationName( QStringLiteral( "Veyon Solutions" ) );
	QCoreApplication::setOrganizationDomain( QStringLiteral( "veyon.io" ) );
	QCoreApplication::setApplicationName( QStringLiteral( "Veyon" ) );

	QCoreApplication::setAttribute( Qt::AA_ShareOpenGLContexts );

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );
#endif
}



QString VeyonCore::applicationName()
{
	return instance()->m_applicationName;
}



void VeyonCore::enforceBranding( QWidget *topLevelWidget )
{
	const auto appName = QStringLiteral( "Veyon" );

	auto labels = topLevelWidget->findChildren<QLabel *>();
	for( auto label : labels )
	{
		label->setText( label->text().replace( appName, VeyonCore::applicationName() ) );
	}

	auto buttons = topLevelWidget->findChildren<QAbstractButton *>();
	for( auto button : buttons )
	{
		button->setText( button->text().replace( appName, VeyonCore::applicationName() ) );
	}

	auto groupBoxes = topLevelWidget->findChildren<QGroupBox *>();
	for( auto groupBox : groupBoxes )
	{
		groupBox->setTitle( groupBox->title().replace( appName, VeyonCore::applicationName() ) );
	}

	auto actions = topLevelWidget->findChildren<QAction *>();
	for( auto action : actions )
	{
		action->setText( action->text().replace( appName, VeyonCore::applicationName() ) );
	}

	auto widgets = topLevelWidget->findChildren<QWidget *>();
	for( auto widget : widgets )
	{
		widget->setWindowTitle( widget->windowTitle().replace( appName, VeyonCore::applicationName() ) );
	}

	topLevelWidget->setWindowTitle( topLevelWidget->windowTitle().replace( appName, VeyonCore::applicationName() ) );
}



bool VeyonCore::isDebugging()
{
	return s_instance && s_instance->m_debugging;
}



QByteArray VeyonCore::shortenFuncinfo( const QByteArray& info )
{
	const auto funcinfo = cleanupFuncinfo( info );

	if( isDebugging() )
	{
		return funcinfo + QByteArrayLiteral( "():" );
	}

	return funcinfo.split( ':' ).first() + QByteArrayLiteral(":");
}



// taken from qtbase/src/corelib/global/qlogging.cpp

QByteArray VeyonCore::cleanupFuncinfo( QByteArray info )
{
	// Strip the function info down to the base function name
	// note that this throws away the template definitions,
	// the parameter types (overloads) and any const/volatile qualifiers.

	if (info.isEmpty())
		return info;

	int pos;

	// Skip trailing [with XXX] for templates (gcc), but make
	// sure to not affect Objective-C message names.
	pos = info.size() - 1;
	if (info.endsWith(']') && !(info.startsWith('+') || info.startsWith('-'))) {
		while (--pos) {
			if (info.at(pos) == '[')
				info.truncate(pos);
		}
	}

	// operator names with '(', ')', '<', '>' in it
	static constexpr std::array<char, 11> operator_call{ "operator()" };
	static constexpr std::array<char, 10> operator_lessThan{ "operator<" };
	static constexpr std::array<char, 10> operator_greaterThan{ "operator>" };
	static constexpr std::array<char, 11> operator_lessThanEqual{ "operator<=" };
	static constexpr std::array<char, 11> operator_greaterThanEqual{ "operator>=" };

	// canonize operator names
	info.replace("operator ", "operator");

	// remove argument list
	Q_FOREVER {
		int parencount = 0;
		pos = info.lastIndexOf(')');
		if (pos == -1) {
			// Don't know how to parse this function name
			return info;
		}

		// find the beginning of the argument list
		--pos;
		++parencount;
		while (pos && parencount) {
			if (info.at(pos) == ')')
				++parencount;
			else if (info.at(pos) == '(')
				--parencount;
			--pos;
		}
		if (parencount != 0)
			return info;

		info.truncate(++pos);

		if (info.at(pos - 1) == ')') {
			if (info.indexOf(operator_call.data()) == pos - int(strlen(operator_call.data())))
				break;

			// this function returns a pointer to a function
			// and we matched the arguments of the return type's parameter list
			// try again
			info.remove(0, info.indexOf('('));
			info.chop(1);
			continue;
		}
		break;
	}

	// find the beginning of the function name
	int parencount = 0;
	int templatecount = 0;
	--pos;

	// make sure special characters in operator names are kept
	if (pos > -1) {
		switch (info.at(pos)) {
		case ')':
			if (info.indexOf(operator_call.data()) == pos - int(strlen(operator_call.data())) + 1)
				pos -= 2;
			break;
		case '<':
			if (info.indexOf(operator_lessThan.data()) == pos - int(strlen(operator_lessThan.data())) + 1)
				--pos;
			break;
		case '>':
			if (info.indexOf(operator_greaterThan.data()) == pos - int(strlen(operator_greaterThan.data())) + 1)
				--pos;
			break;
		case '=': {
			const auto operatorLength = int(strlen(operator_lessThanEqual.data()));
			if (info.indexOf(operator_lessThanEqual.data()) == pos - operatorLength + 1)
				pos -= 2;
			else if (info.indexOf(operator_greaterThanEqual.data()) == pos - operatorLength + 1)
				pos -= 2;
			break;
		}
		default:
			break;
		}
	}

	while (pos > -1) {
		if (parencount < 0 || templatecount < 0)
			return info;

		char c = info.at(pos);
		if (c == ')')
			++parencount;
		else if (c == '(')
			--parencount;
		else if (c == '>')
			++templatecount;
		else if (c == '<')
			--templatecount;
		else if (c == ' ' && templatecount == 0 && parencount == 0)
			break;

		--pos;
	}
	info = info.mid(pos + 1);

	// remove trailing '*', '&' that are part of the return argument
	while ((info.at(0) == '*')
		   || (info.at(0) == '&'))
		info = info.mid(1);

	// we have the full function name now.
	// clean up the templates
	while ((pos = info.lastIndexOf('>')) != -1) {
		if (!info.contains('<'))
			break;

		// find the matching close
		int end = pos;
		templatecount = 1;
		--pos;
		while (pos && templatecount) {
			char c = info.at(pos);
			if (c == '>')
				++templatecount;
			else if (c == '<')
				--templatecount;
			--pos;
		}
		++pos;
		info.remove(pos, end - pos + 1);
	}

	return info;
}



QString VeyonCore::stripDomain( const QString& username )
{
	// remove the domain part of username (e.g. "EXAMPLE.COM\Teacher" -> "Teacher")
	int domainSeparator = username.indexOf( QLatin1Char('\\') );
	if( domainSeparator >= 0 )
	{
		return username.mid( domainSeparator + 1 );
	}

	return username;
}



QString VeyonCore::formattedUuid( QUuid uuid )
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	return uuid.toString( QUuid::WithoutBraces );
#else
	return uuid.toString().remove( QLatin1Char('{') ).remove( QLatin1Char('}') );
#endif
}



QString VeyonCore::stringify( const QVariantMap& map )
{
	return QString::fromUtf8( QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact) );
}



QString VeyonCore::screenName(const QScreen& screen, int index)
{
	auto screenName = tr("Screen %1").arg(index);

	const auto displayDeviceName = platform().coreFunctions().queryDisplayDeviceName(screen);
	if(displayDeviceName.isEmpty() == false)
	{
		screenName.append(QStringLiteral(" – %1").arg(displayDeviceName));
	}

	return screenName;
}



int VeyonCore::exec()
{
	Q_EMIT applicationLoaded();

	vDebug() << "Running";

	const auto result = QCoreApplication::exec();

	vDebug() << "Exit";

	return result;
}



void VeyonCore::initPlatformPlugin()
{
	// initialize plugin manager and load platform plugins first
	m_pluginManager = new PluginManager( this );
	m_pluginManager->loadPlatformPlugins();

	// initialize platform plugin manager and initialize used platform plugin
	m_platformPluginManager = new PlatformPluginManager( *m_pluginManager );
	m_platformPlugin = m_platformPluginManager->platformPlugin();
}



void VeyonCore::initSession()
{
	if( component() != Component::Service && config().multiSessionModeEnabled() )
	{
		const auto systemEnv = QProcessEnvironment::systemEnvironment();
		if( systemEnv.contains( sessionIdEnvironmentVariable() ) )
		{
			m_sessionId = systemEnv.value( sessionIdEnvironmentVariable() ).toInt();
		}
		else
		{
			const auto sessionId = platform().sessionFunctions().currentSessionId();
			if( sessionId != PlatformSessionFunctions::InvalidSessionId )
			{
				m_sessionId = sessionId;
			}
		}
	}
	else
	{
		m_sessionId = PlatformSessionFunctions::DefaultSessionId;
	}
}


void VeyonCore::initConfiguration()
{
	m_config = new VeyonConfiguration();
	m_config->upgrade();

	if( QUuid( config().installationID() ).isNull() )
	{
		config().setInstallationID( formattedUuid( QUuid::createUuid() ) );
	}

	if( config().applicationName().isEmpty() == false )
	{
		m_applicationName = config().applicationName();
	}
}



void VeyonCore::initLogging( const QString& appComponentName )
{
	const auto currentSessionId = sessionId();

	if( currentSessionId != PlatformSessionFunctions::DefaultSessionId )
	{
		m_logger = new Logger( QStringLiteral("%1-%2").arg( appComponentName ).arg( currentSessionId ) );
	}
	else
	{
		m_logger = new Logger( appComponentName );
	}

	m_debugging = ( m_logger->logLevel() >= Logger::LogLevel::Debug );

	VncConnection::initLogging( isDebugging() );
}



void VeyonCore::initLocaleAndTranslation()
{
	if( TranslationLoader::load( QStringLiteral("qtbase") ) == false )
	{
		TranslationLoader::load( QStringLiteral("qt") );
	}

	TranslationLoader::load( QStringLiteral("veyon") );

	const auto app = qobject_cast<QGuiApplication *>( QCoreApplication::instance() );
	if( app )
	{
		QGuiApplication::setLayoutDirection( QLocale{}.textDirection() );
	}
}



void VeyonCore::initUi()
{
	auto app = qobject_cast<QApplication *>(QCoreApplication::instance());
	if (app)
	{
		if (m_config->uiStyle() == UiStyle::Fusion)
		{
			app->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
		}

		app->setStyleSheet(QStringLiteral(
							   "QToolButton:checked {background-color:#88ddff;}"
							   "QToolTip {color:#ffffff; background-color:#198cb3; padding:5px; border:0px;}"
							   ));
	}
}



void VeyonCore::initCryptoCore()
{
	m_cryptoCore = new CryptoCore;
}



void VeyonCore::initQmlCore()
{
	m_qmlCore = new QmlCore( this );
}



void VeyonCore::initAuthenticationCredentials()
{
	if( m_authenticationCredentials )
	{
		delete m_authenticationCredentials;
		m_authenticationCredentials = nullptr;
	}

	m_authenticationCredentials = new AuthenticationCredentials;
}



void VeyonCore::initPlugins()
{
	// load all other (non-platform) plugins
	m_pluginManager->loadPlugins();
	m_pluginManager->upgradePlugins();

	m_builtinFeatures = new BuiltinFeatures();
}



void VeyonCore::initManagers()
{
	m_authenticationManager = new AuthenticationManager( this );
	m_featureManager = new FeatureManager(this);
	m_userGroupsBackendManager = new UserGroupsBackendManager( this );
	m_networkObjectDirectoryManager = new NetworkObjectDirectoryManager( this );
}



void VeyonCore::initSystemInfo()
{
	vDebug() << versionString() << HostAddress::localFQDN()
			 << QSysInfo::kernelType() << QSysInfo::kernelVersion()
			 << QSysInfo::prettyProductName() << QSysInfo::productType() << QSysInfo::productVersion();
}



void VeyonCore::initTlsConfiguration()
{
	auto tlsConfig{TlsConfiguration::defaultConfiguration()};

#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
	tlsConfig.setProtocol( QSsl::TlsV1_3OrLater );
#else
	tlsConfig.setProtocol( QSsl::TlsV1_2OrLater );
#endif

	if( config().tlsUseCertificateAuthority() )
	{
		loadCertificateAuthorityFiles( &tlsConfig );
	}
	else
	{
		addSelfSignedHostCertificate( &tlsConfig );
	}

	TlsConfiguration::setDefaultConfiguration( tlsConfig );
}



bool VeyonCore::loadCertificateAuthorityFiles( TlsConfiguration* tlsConfig )
{
	QFile caCertFile( filesystem().expandPath( config().tlsCaCertificateFile() ) );

	if( caCertFile.exists() == false )
	{
		vCritical() << "TLS CA certificate file" << caCertFile.fileName() << "does not exist";
		return false;
	}

	if( caCertFile.open( QFile::ReadOnly ) == false )
	{
		vCritical() << "TLS CA certificate file" << caCertFile.fileName() << "is not readable";
		return false;
	}

	QSslCertificate caCertificate( caCertFile.readAll() );
	if( caCertificate.isNull() )
	{
		vCritical() << caCertFile.fileName() << "does not contain a valid TLS certificate";
		return false;
	}

	QFile hostCertFile( filesystem().expandPath( config().tlsHostCertificateFile() ) );

	if( hostCertFile.exists() == false )
	{
		vCritical() << "TLS host certificate file" << hostCertFile.fileName() << "does not exist";
		return false;
	}

	if( hostCertFile.open( QFile::ReadOnly ) == false )
	{
		vCritical() << "TLS host certificate file" << hostCertFile.fileName() << "is not readable";
		return false;
	}

	QSslCertificate hostCertificate( hostCertFile.readAll() );
	if( hostCertificate.isNull() )
	{
		vCritical() << hostCertFile.fileName() << "does not contain a valid TLS certificate";
		return false;
	}

	QFile hostPrivateKeyFile( filesystem().expandPath( config().tlsHostPrivateKeyFile() ) );

	if( hostPrivateKeyFile.exists() == false )
	{
		vCritical() << "TLS private key file" << hostPrivateKeyFile.fileName() << "does not exist";
		return false;
	}

	if( hostPrivateKeyFile.open( QFile::ReadOnly ) == false )
	{
		vCritical() << "TLS private key file" << hostPrivateKeyFile.fileName() << "is not readable";
		return false;
	}

	const auto hostPrivateKeyFileData = hostPrivateKeyFile.readAll();

	QSslKey hostPrivateKey;
	for( auto algorithm : { QSsl::KeyAlgorithm::Rsa, QSsl::KeyAlgorithm::Ec
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
				 , QSsl::KeyAlgorithm::Dh
#endif
		 } )
	{
		QSslKey currentPrivateKey( hostPrivateKeyFileData, algorithm );
		if( currentPrivateKey.isNull() == false )
		{
			hostPrivateKey = currentPrivateKey;
			break;
		}
	}

	if( hostPrivateKey.isNull() )
	{
		vCritical() << hostPrivateKeyFile.fileName() << "does contains an invalid or unsupported TLS private key";
		return false;
	}

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	tlsConfig->addCaCertificates( TlsConfiguration::systemCaCertificates() );
	tlsConfig->addCaCertificate( caCertificate );
#else
	tlsConfig->setCaCertificates( TlsConfiguration::systemCaCertificates() + QList<QSslCertificate>{caCertificate} );
#endif
	tlsConfig->setLocalCertificate( hostCertificate );
	tlsConfig->setPrivateKey( hostPrivateKey );

	return true;
}



bool VeyonCore::addSelfSignedHostCertificate( TlsConfiguration* tlsConfig )
{
	const auto privateKey = cryptoCore().createPrivateKey();
	if( privateKey.isNull() )
	{
		vCritical() << "failed to create private key for host certificate";
		return false;
	}

	const auto cert = cryptoCore().createSelfSignedHostCertificate( privateKey );
	if( cert.isNull() )
	{
		vCritical() << "failed to create host certificate";
		return false;
	}

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	tlsConfig->addCaCertificates( TlsConfiguration::systemCaCertificates() );
#else
	tlsConfig->setCaCertificates( TlsConfiguration::systemCaCertificates() );
#endif
	tlsConfig->setLocalCertificate( QSslCertificate( cert.toPEM().toUtf8() ) );
	tlsConfig->setPrivateKey( QSslKey( privateKey.toPEM().toUtf8(), QSsl::KeyAlgorithm::Rsa ) );

	return true;
}
