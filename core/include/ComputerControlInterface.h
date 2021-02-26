/*
 * ComputerControlInterface.h - interface class for controlling a computer
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QList>
#include <QObject>
#include <QSize>
#include <QTimer>

#include "Computer.h"
#include "Feature.h"
#include "Lockable.h"
#include "VeyonCore.h"
#include "VncConnection.h"

class QImage;

class AuthenticationProxy;
class FeatureMessage;
class VncConnection;
class VeyonConnection;

class VEYON_CORE_EXPORT ComputerControlInterface : public QObject, public Lockable
{
	Q_OBJECT
public:
	enum class UpdateMode {
		Disabled,
		Monitoring,
		Live
	};

	using Pointer = QSharedPointer<ComputerControlInterface>;

	using State = VncConnection::State;

	explicit ComputerControlInterface( const Computer& computer, QObject* parent = nullptr );
	~ComputerControlInterface() override;

	void start( QSize scaledScreenSize = {}, UpdateMode updateMode = UpdateMode::Monitoring, AuthenticationProxy* authenticationProxy = nullptr );
	void stop();

	const Computer& computer() const
	{
		return m_computer;
	}

	State state() const
	{
		return m_state;
	}

	bool hasValidFramebuffer() const;

	QSize screenSize() const;

	const QSize& scaledScreenSize() const
	{
		return m_scaledScreenSize;
	}

	void setScaledScreenSize( QSize size );

	QImage scaledScreen() const;

	QImage screen() const;

	const QString& userLoginName() const
	{
		return m_userLoginName;
	}

	const QString& userFullName() const
	{
		return m_userFullName;
	}

	int userSessionId() const
	{
		return m_userSessionId;
	}

	void setUserInformation( const QString& userLoginName, const QString& userFullName, int sessionId );

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

	void updateActiveFeatures();

	void sendFeatureMessage( const FeatureMessage& featureMessage, bool wake );
	bool isMessageQueueEmpty();

	void setUpdateMode( UpdateMode updateMode );
	UpdateMode updateMode() const
	{
		return m_updateMode;
	}

	Pointer weakPointer();

private:
	void resetWatchdog();
	void restartConnection();

	void updateState();
	void updateUser();

	void handleFeatureMessage( const FeatureMessage& message );

	static constexpr int ConnectionWatchdogTimeout = 10000;
	static constexpr int UpdateIntervalDisabled = 5000;

	Computer m_computer;

	UpdateMode m_updateMode{UpdateMode::Disabled};

	State m_state;
	QString m_userLoginName;
	QString m_userFullName;
	int m_userSessionId{0};
	FeatureUidList m_activeFeatures;
	Feature::Uid m_designatedModeFeature;

	QSize m_scaledScreenSize;

	VncConnection* m_vncConnection;
	VeyonConnection* m_connection;
	QTimer m_connectionWatchdogTimer;
	QTimer m_userUpdateTimer;
	QTimer m_activeFeaturesUpdateTimer;

Q_SIGNALS:
	void featureMessageReceived( const FeatureMessage&, ComputerControlInterface::Pointer );
	void screenSizeChanged();
	void screenUpdated();
	void userChanged();
	void stateChanged();
	void activeFeaturesChanged();

};

using ComputerControlInterfaceList = QVector<ComputerControlInterface::Pointer>;

Q_DECLARE_METATYPE(ComputerControlInterface::Pointer)
