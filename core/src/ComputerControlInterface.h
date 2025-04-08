/*
 * ComputerControlInterface.h - interface class for controlling a computer
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

#pragma once

#include "Computer.h"
#include "Feature.h"
#include "Lockable.h"
#include "PlatformSessionFunctions.h"
#include "VeyonCore.h"
#include "VeyonConnection.h"

class VEYON_CORE_EXPORT ComputerControlInterface : public QObject, public Lockable
{
	Q_OBJECT
public:
	enum class UpdateMode {
		Disabled,
		Basic,
		Monitoring,
		Live,
		FeatureControlOnly,
	};

	using Pointer = QSharedPointer<ComputerControlInterface>;

	using State = VncConnection::State;

	struct ScreenProperties {
		int index;
		QString name;
		QRect geometry;
		bool operator==(const ScreenProperties& other) const
		{
			return other.index == index &&
				   other.name == name &&
				   other.geometry == geometry;
		}
	};
	using ScreenList = QList<ScreenProperties>;

	explicit ComputerControlInterface( const Computer& computer, int port = -1, QObject* parent = nullptr );
	~ComputerControlInterface() override;

	VeyonConnection* connection() const
	{
		return m_connection;
	}

	VncConnection* vncConnection() const
	{
		return m_connection ? m_connection->vncConnection() : nullptr;
	}

	void start( QSize scaledFramebufferSize = {}, UpdateMode updateMode = UpdateMode::Monitoring );
	void stop();

	const Computer& computer() const
	{
		return m_computer;
	}

	QString computerName() const;

	Computer::NameSource computerNameSource() const
	{
		return m_computerNameSource;
	}

	State state() const
	{
		return m_state;
	}

	bool hasValidFramebuffer() const;

	QSize screenSize() const;

	const QSize& scaledFramebufferSize() const
	{
		return m_scaledFramebufferSize;
	}

	void setScaledFramebufferSize( QSize size );

	QImage scaledFramebuffer() const;

	QImage framebuffer() const;

	int timestamp() const
	{
		return m_timestamp;
	}

	const QString& accessControlDetails() const
	{
		return m_accessControlDetails;
	}

	void setAccessControlFailed(const QString& details);

	VeyonCore::ApplicationVersion serverVersion() const
	{
		return m_serverVersion;
	}

	void setServerVersion(VeyonCore::ApplicationVersion version);

	const QString& userLoginName() const
	{
		return m_userLoginName;
	}

	const QString& userFullName() const
	{
		return m_userFullName;
	}

	void setUserInformation(const QString& userLoginName, const QString& userFullName);

	const PlatformSessionFunctions::SessionInfo& sessionInfo() const
	{
		return m_sessionInfo;
	}

	void setSessionInfo(const PlatformSessionFunctions::SessionInfo& sessionInfo);

	const ScreenList& screens() const
	{
		return m_screens;
	}

	void setScreens(const ScreenList& screens);

	const FeatureUidList& activeFeatures() const
	{
		return m_activeFeatures;
	}

	void setActiveFeatures( const FeatureUidList& activeFeatures );

	Feature::Uid designatedModeFeature() const
	{
		return m_designatedModeFeature;
	}

	void setDesignatedModeFeature( Feature::Uid designatedModeFeature )
	{
		m_designatedModeFeature = designatedModeFeature;
	}

	const QStringList& groups() const
	{
		return m_groups;
	}

	void setGroups( const QStringList& groups )
	{
		m_groups = groups;
	}

	void sendFeatureMessage(const FeatureMessage& featureMessage);
	bool isMessageQueueEmpty();

	void setUpdateMode( UpdateMode updateMode );
	UpdateMode updateMode() const
	{
		return m_updateMode;
	}

	void setProperty(QUuid propertyId, const QVariant& data);

	QVariant queryProperty(QUuid propertyId);

	Pointer weakPointer();

	template<typename T>
	void executeIfConnected(const T& functor)
	{
		connect(this, &ComputerControlInterface::stateChanged, this, [this, functor]() {
			if (state() == State::Connected)
			{
				functor();
			}
		});
	}

private:
	void ping();
	void setMinimumFramebufferUpdateInterval();
	void setQuality();
	void resetWatchdog();
	void restartConnection();

	void updateState();
	void updateServerVersion();
	void updateActiveFeatures();
	void updateUser();
	void updateSessionInfo();
	void updateScreens();

	void handleFeatureMessage( const FeatureMessage& message );

	static constexpr int ConnectionWatchdogPingDelay = 10000;
	static constexpr int ConnectionWatchdogTimeout = ConnectionWatchdogPingDelay*2;
	static constexpr int ServerVersionQueryTimeout = 5000;
	static constexpr int UpdateIntervalDisabled = 5000;

	const Computer m_computer;
	const int m_port;

	UpdateMode m_updateMode{UpdateMode::Disabled};
	Computer::NameSource m_computerNameSource{Computer::NameSource::Default};

	State m_state{State::Disconnected};
	QString m_userLoginName{};
	QString m_userFullName{};
	PlatformSessionFunctions::SessionInfo m_sessionInfo{};
	ScreenList m_screens;
	FeatureUidList m_activeFeatures;
	Feature::Uid m_designatedModeFeature;

	QSize m_scaledFramebufferSize{};
	int m_timestamp{0};

	VeyonConnection* m_connection{nullptr};
	QTimer m_pingTimer{this};
	QTimer m_connectionWatchdogTimer{this};

	VeyonCore::ApplicationVersion m_serverVersion{VeyonCore::ApplicationVersion::Unknown};
	QTimer m_serverVersionQueryTimer{this};

	QString m_accessControlDetails{};
	QTimer m_statePollingTimer{this};

	QStringList m_groups;

	QMap<QUuid, QVariant> m_properties;

Q_SIGNALS:
	void accessControlDetailsChanged();
	void framebufferSizeChanged();
	void framebufferUpdated( QRect rect );
	void scaledFramebufferUpdated();
	void userChanged();
	void sessionInfoChanged();
	void screensChanged();
	void stateChanged();
	void activeFeaturesChanged();
	void propertyChanged(QUuid propertyId);

};

using ComputerControlInterfaceList = QVector<ComputerControlInterface::Pointer>;

VEYON_CORE_EXPORT QDebug operator<<(QDebug stream, ComputerControlInterface::Pointer computerControlInterface);
VEYON_CORE_EXPORT QDebug operator<<(QDebug stream, const ComputerControlInterfaceList& computerControlInterfaces);

Q_DECLARE_METATYPE(ComputerControlInterface::Pointer)
