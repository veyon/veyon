/*
 * ComputerControlInterface.cpp - interface class for controlling a computer
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "AccessControlProvider.h"
#include "BuiltinFeatures.h"
#include "ComputerControlInterface.h"
#include "Computer.h"
#include "FeatureManager.h"
#include "MonitoringMode.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VncConnection.h"


ComputerControlInterface::ComputerControlInterface( const Computer& computer, int port, QObject* parent ) :
	QObject( parent ),
	m_computer( computer ),
	m_port(port),
	m_computerNameSource(VeyonCore::config().computerNameSource())
{
	m_pingTimer.setInterval(ConnectionWatchdogPingDelay);
	m_pingTimer.setSingleShot(true);
	connect(&m_pingTimer, &QTimer::timeout, this, &ComputerControlInterface::ping);

	m_connectionWatchdogTimer.setInterval( ConnectionWatchdogTimeout );
	m_connectionWatchdogTimer.setSingleShot( true );
	connect( &m_connectionWatchdogTimer, &QTimer::timeout, this, &ComputerControlInterface::restartConnection );

	m_serverVersionQueryTimer.setInterval(ServerVersionQueryTimeout);
	m_serverVersionQueryTimer.setSingleShot(true);
	connect( &m_serverVersionQueryTimer, &QTimer::timeout, this, [this]() {
		setServerVersion(VeyonCore::ApplicationVersion::Unknown);
	});

	connect(&m_statePollingTimer, &QTimer::timeout, this, [this]() {
		updateUser();
		updateSessionInfo();
		updateActiveFeatures();
	});
}



ComputerControlInterface::~ComputerControlInterface()
{
	stop();
}



void ComputerControlInterface::start( QSize scaledFramebufferSize, UpdateMode updateMode )
{
	// make sure we do not leak
	stop();

	m_scaledFramebufferSize = scaledFramebufferSize;

	if (m_computer.hostName().isEmpty() == false)
	{
		m_connection = new VeyonConnection;

		auto vncConnection = m_connection->vncConnection();
		vncConnection->setHost(m_computer.hostName());
		if( m_port > 0 )
		{
			vncConnection->setPort( m_port );
		}
		vncConnection->setScaledSize( m_scaledFramebufferSize );

		connect( vncConnection, &VncConnection::imageUpdated, this, [this]( int x, int y, int w, int h )
		{
			Q_EMIT framebufferUpdated( QRect( x, y, w, h ) );
		} );
		connect( vncConnection, &VncConnection::framebufferUpdateComplete, this, [this]() {
			resetWatchdog();
			++m_timestamp;
			Q_EMIT scaledFramebufferUpdated();
		} );

		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateState );
		connect(vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::setMinimumFramebufferUpdateInterval);
		connect(vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateServerVersion);
		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateUser );
		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateSessionInfo );
		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateActiveFeatures );
		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateScreens );
		connect( vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::stateChanged );

		connect( m_connection, &VeyonConnection::featureMessageReceived, this, &ComputerControlInterface::handleFeatureMessage );
		connect( m_connection, &VeyonConnection::featureMessageReceived, this, &ComputerControlInterface::resetWatchdog );

		setUpdateMode( updateMode );

		vncConnection->start();
	}
	else
	{
		vWarning() << "computer host address is empty!";
	}
}



void ComputerControlInterface::stop()
{
	if( m_connection )
	{
		m_connection->stopAndDeleteLater();
		m_connection = nullptr;
	}

	m_pingTimer.stop();
	m_connectionWatchdogTimer.stop();

	m_state = State::Disconnected;
}



QString ComputerControlInterface::computerName() const
{
	const auto propertyOrFallback = [this](const QString& value)
	{
		return value.isEmpty() ?
					(computer().displayName().isEmpty() ? computer().hostName() : computer().displayName())
				  :
					value;
	};

	switch (computerNameSource())
	{
	case Computer::NameSource::HostAddress: return propertyOrFallback(computer().hostName());
	case Computer::NameSource::SessionClientName: return propertyOrFallback(sessionInfo().clientName);
	case Computer::NameSource::SessionClientAddress: return propertyOrFallback(sessionInfo().clientAddress);
	case Computer::NameSource::SessionHostName: return propertyOrFallback(sessionInfo().hostName);
	case Computer::NameSource::SessionMetaData: return propertyOrFallback(sessionInfo().metaData);
	case Computer::NameSource::UserFullName: return propertyOrFallback(userFullName());
	case Computer::NameSource::UserLoginName: return propertyOrFallback(userLoginName());
	default:
		break;
	}

	return propertyOrFallback({});
}



bool ComputerControlInterface::hasValidFramebuffer() const
{
	return vncConnection() && vncConnection()->hasValidFramebuffer();
}



QSize ComputerControlInterface::screenSize() const
{
	if( vncConnection() )
	{
		return vncConnection()->image().size();
	}

	return {};
}



void ComputerControlInterface::setScaledFramebufferSize( QSize scaledFramebufferSize )
{
	m_scaledFramebufferSize = scaledFramebufferSize;

	if( vncConnection() )
	{
		vncConnection()->setScaledSize( m_scaledFramebufferSize );
	}

	++m_timestamp;

	Q_EMIT scaledFramebufferUpdated();
}



QImage ComputerControlInterface::scaledFramebuffer() const
{
	if( vncConnection() && vncConnection()->isConnected() )
	{
		return vncConnection()->scaledFramebuffer();
	}

	return {};
}



QImage ComputerControlInterface::framebuffer() const
{
	if( vncConnection() && vncConnection()->isConnected() )
	{
		return vncConnection()->image();
	}

	return {};
}



void ComputerControlInterface::setAccessControlFailed(const QString& details)
{
	lock();
	m_accessControlDetails = details;
	m_state = State::AccessControlFailed;

	if (vncConnection())
	{
		vncConnection()->stop();
	}
	unlock();

	Q_EMIT accessControlDetailsChanged();
	Q_EMIT stateChanged();
}



void ComputerControlInterface::setServerVersion(VeyonCore::ApplicationVersion version)
{
	m_serverVersionQueryTimer.stop();

	m_serverVersion = version;

	const auto statePollingInterval = VeyonCore::config().computerStatePollingInterval();

	setQuality();

	if (m_serverVersion >= VeyonCore::ApplicationVersion::Version_4_7 &&
		statePollingInterval <= 0)
	{
		m_statePollingTimer.stop();

		updateSessionInfo();
		updateScreens();
		setMinimumFramebufferUpdateInterval();
	}
	else
	{
		if (vncConnection())
		{
			vncConnection()->setRequiresManualUpdateRateControl(true);
		}

		m_statePollingTimer.start(statePollingInterval > 0 ? statePollingInterval :
															 VeyonCore::config().computerMonitoringUpdateInterval());
	}
}



void ComputerControlInterface::setUserInformation(const QString& userLoginName, const QString& userFullName)
{
	if (userLoginName != m_userLoginName ||
		userFullName != m_userFullName)
	{
		m_userLoginName = userLoginName;
		m_userFullName = userFullName;

		Q_EMIT userChanged();
	}
}



void ComputerControlInterface::setSessionInfo(const PlatformSessionFunctions::SessionInfo& sessionInfo)
{
	if (sessionInfo != m_sessionInfo)
	{
		m_sessionInfo = sessionInfo;
		Q_EMIT sessionInfoChanged();
	}
}



void ComputerControlInterface::setScreens(const ScreenList& screens)
{
	if(screens != m_screens)
	{
		m_screens = screens;

		Q_EMIT screensChanged();
	}
}



void ComputerControlInterface::setActiveFeatures( const FeatureUidList& activeFeatures )
{
	if( activeFeatures != m_activeFeatures )
	{
		m_activeFeatures = activeFeatures;

		Q_EMIT activeFeaturesChanged();
	}
}



void ComputerControlInterface::sendFeatureMessage(const FeatureMessage& featureMessage)
{
	if( m_connection && m_connection->isConnected() )
	{
		m_connection->sendFeatureMessage(featureMessage);
	}
}



bool ComputerControlInterface::isMessageQueueEmpty()
{
	if( vncConnection() && vncConnection()->isConnected() )
	{
		return vncConnection()->isEventQueueEmpty();
	}

	return true;
}



void ComputerControlInterface::setUpdateMode( UpdateMode updateMode )
{
	m_updateMode = updateMode;

	setMinimumFramebufferUpdateInterval();
	setQuality();

	if (vncConnection())
	{
		vncConnection()->setSkipHostPing(m_updateMode == UpdateMode::Basic || m_updateMode == UpdateMode::FeatureControlOnly);
	}
}



void ComputerControlInterface::setProperty(QUuid propertyId, const QVariant& data)
{
	if (propertyId.isNull() == false)
	{
		lock();
		m_properties[propertyId] = data;
		unlock();

		Q_EMIT propertyChanged(propertyId);
	}
}



QVariant ComputerControlInterface::queryProperty(QUuid propertyId)
{
	lock();
	const auto data = m_properties.value(propertyId);
	unlock();

	return data;
}



ComputerControlInterface::Pointer ComputerControlInterface::weakPointer()
{
	return Pointer( this, []( ComputerControlInterface* ) { } );
}



void ComputerControlInterface::ping()
{
	if (m_serverVersion >= VeyonCore::ApplicationVersion::Version_4_7)
	{
		VeyonCore::builtinFeatures().monitoringMode().ping({weakPointer()});
	}
}



void ComputerControlInterface::setMinimumFramebufferUpdateInterval()
{
	auto updateInterval = -1;

	switch (m_updateMode)
	{
	case UpdateMode::Disabled:
		updateInterval = UpdateIntervalDisabled;
		break;

	case UpdateMode::Basic:
	case UpdateMode::Monitoring:
		updateInterval = VeyonCore::config().computerMonitoringUpdateInterval();
		break;

	case UpdateMode::Live:
		break;

	case UpdateMode::FeatureControlOnly:
		if (vncConnection())
		{
			vncConnection()->setSkipFramebufferUpdates(true);
		}
		break;
	}

	if (vncConnection())
	{
		vncConnection()->setFramebufferUpdateInterval(updateInterval);
	}

	if (m_serverVersion >= VeyonCore::ApplicationVersion::Version_4_7)
	{
		VeyonCore::builtinFeatures().monitoringMode().setMinimumFramebufferUpdateInterval({weakPointer()}, updateInterval);
	}
}



void ComputerControlInterface::setQuality()
{
	auto quality = VncConnectionConfiguration::Quality::Highest;

	if (m_serverVersion >= VeyonCore::ApplicationVersion::Version_4_8)
	{
		switch (m_updateMode)
		{
		case UpdateMode::Disabled:
		case UpdateMode::FeatureControlOnly:
			quality = VncConnectionConfiguration::Quality::Lowest;
			break;

		case UpdateMode::Basic:
		case UpdateMode::Monitoring:
			quality = VeyonCore::config().computerMonitoringImageQuality();
			break;

		case UpdateMode::Live:
			quality = VeyonCore::config().remoteAccessImageQuality();
			break;
		}
	}

	if (vncConnection())
	{
		vncConnection()->setQuality(quality);
	}
}



void ComputerControlInterface::resetWatchdog()
{
	if (state() == State::Connected || state() == State::AccessControlFailed)
	{
		m_pingTimer.start();
		m_connectionWatchdogTimer.start();
	}
}



void ComputerControlInterface::restartConnection()
{
	if( vncConnection() )
	{
		vDebug();
		vncConnection()->restart();

		m_connectionWatchdogTimer.stop();
	}
}



void ComputerControlInterface::updateState()
{
	lock();

	if (vncConnection())
	{
		if (!(m_state == State::AccessControlFailed && vncConnection()->state() == VncConnection::State::Disconnected))
		{
			m_state = vncConnection()->state();
		}
	}
	else
	{
		m_state = State::Disconnected;
	}

	unlock();
}



void ComputerControlInterface::updateServerVersion()
{
	lock();

	if (vncConnection())
	{
		VeyonCore::builtinFeatures().monitoringMode().queryApplicationVersion({weakPointer()});
		m_serverVersionQueryTimer.start();
	}

	unlock();
}



void ComputerControlInterface::updateActiveFeatures()
{
	lock();

	if (vncConnection() && state() == State::Connected)
	{
		VeyonCore::builtinFeatures().monitoringMode().queryActiveFeatures({weakPointer()});
	}
	else
	{
		setActiveFeatures({});
	}

	unlock();
}



void ComputerControlInterface::updateUser()
{
	lock();

	if( vncConnection() && state() == State::Connected )
	{
		if( userLoginName().isEmpty() )
		{
			VeyonCore::builtinFeatures().monitoringMode().queryUserInfo( { weakPointer() } );
		}
	}
	else
	{
		setUserInformation({}, {});
	}

	unlock();
}



void ComputerControlInterface::updateSessionInfo()
{
	lock();

	if (vncConnection() && state() == State::Connected &&
		m_serverVersion >= VeyonCore::ApplicationVersion::Version_4_8)
	{
		VeyonCore::builtinFeatures().monitoringMode().querySessionInfo({weakPointer()});
	}
	else
	{
		setSessionInfo({});
	}

	unlock();
}



void ComputerControlInterface::updateScreens()
{
	lock();

	if (vncConnection() && state() == State::Connected &&
		m_serverVersion >= VeyonCore::ApplicationVersion::Version_4_7)
	{
		VeyonCore::builtinFeatures().monitoringMode().queryScreens({weakPointer()});
	}
	else
	{
		setScreens({});
	}

	unlock();
}



void ComputerControlInterface::handleFeatureMessage( const FeatureMessage& message )
{
	lock();
	VeyonCore::featureManager().handleFeatureMessage( weakPointer(), message );
	unlock();
}



QDebug operator<<(QDebug stream, ComputerControlInterface::Pointer computerControlInterface)
{
	if (computerControlInterface.isNull() == false)
	{
		stream << qUtf8Printable(computerControlInterface->computer().hostName());
	}
	return stream;
}



QDebug operator<<(QDebug stream, const ComputerControlInterfaceList& computerControlInterfaces)
{
	QStringList hostAddresses;
	hostAddresses.reserve(computerControlInterfaces.size());
	for(const auto& computerControlInterface : computerControlInterfaces)
	{
		if (computerControlInterface.isNull() == false)
		{
			hostAddresses.append(computerControlInterface->computer().hostName());
		}
	}

	stream << QStringLiteral("[%1]").arg(hostAddresses.join(QLatin1Char(','))).toUtf8().constData();

	return stream;
}
