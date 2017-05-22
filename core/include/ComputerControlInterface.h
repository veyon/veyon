/*
 * ComputerControlInterface.h - interface class for controlling a computer
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef COMPUTER_CONTROL_INTERFACE_H
#define COMPUTER_CONTROL_INTERFACE_H

#include <QList>
#include <QObject>
#include <QSize>

#include "VeyonCore.h"

class QImage;

class Computer;
class FeatureMessage;
class VeyonVncConnection;
class VeyonCoreConnection;
class UserSessionControl;

class VEYON_CORE_EXPORT ComputerControlInterface : public QObject
{
	Q_OBJECT
public:
	typedef enum States
	{
		Disconnected,
		Offline,
		ServiceUnreachable,
		AuthenticationFailed,
		Connecting,
		Connected,
		Unknown,
		StateCount
	} State;

	ComputerControlInterface( const Computer& computer );
	~ComputerControlInterface() override;

	void start( const QSize& scaledScreenSize, UserSessionControl* userSessionControl );
	void stop();

	const Computer& computer() const
	{
		return m_computer;
	}

	State state() const
	{
		return m_state;
	}

	const QSize& scaledScreenSize() const
	{
		return m_scaledScreenSize;
	}

	void setScaledScreenSize( const QSize& size );

	QImage scaledScreen() const;

	QImage screen() const;

	bool hasScreenUpdates() const
	{
		return m_screenUpdated;
	}

	void clearScreenUpdateFlag()
	{
		m_screenUpdated = false;
	}

	const QString& user() const
	{
		return m_user;
	}

	void setUser( const QString& user );

	void sendFeatureMessage( const FeatureMessage& featureMessage );


private slots:
	void setScreenUpdateFlag()
	{
		m_screenUpdated = true;
	}

	void updateState();
	void updateUser();

	void handleFeatureMessage( const FeatureMessage& message );

private:
	enum {
		FramebufferUpdateInterval = 1000,
	};

	const Computer& m_computer;

	State m_state;
	QString m_user;

	QSize m_scaledScreenSize;

	VeyonVncConnection* m_vncConnection;
	VeyonCoreConnection* m_coreConnection;
	UserSessionControl* m_userSessionControl;

	bool m_screenUpdated;

signals:
	void featureMessageReceived( const FeatureMessage&, ComputerControlInterface& );
	void userChanged();

};

typedef QList<ComputerControlInterface *> ComputerControlInterfaceList;

#endif // COMPUTER_CONTROL_INTERFACE_H
