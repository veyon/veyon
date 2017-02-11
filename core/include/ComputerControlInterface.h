/*
 * ComputerControlInterface.h - interface class for controlling a computer
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

class QImage;

class Computer;
class FeatureMessage;
class ItalcVncConnection;
class ItalcCoreConnection;

class ComputerControlInterface : public QObject
{
	Q_OBJECT
public:
	typedef enum Modes
	{
		ModeMonitoring,
		ModeFullScreenDemo,
		ModeWindowDemo,
		ModeLocked,
		ModeCount
	} Mode;

	typedef enum States
	{
		Disconnected,
		Unreachable,
		Connecting,
		Connected,
		Unknown,
		StateCount
	} State;

	ComputerControlInterface( const Computer& computer );
	~ComputerControlInterface() override;

	void start( const QSize& screenSize );
	void stop();

	State state() const
	{
		return m_state;
	}

	Mode mode() const
	{
		return m_mode;
	}

	const QSize& screenSize() const
	{
		return m_screenSize;
	}

	void setScreenSize( const QSize& size );

	QImage screen() const;

	bool hasScreenUpdates() const
	{
		return m_screenUpdated;
	}

	void clearScreenUpdateFlag()
	{
		m_screenUpdated = false;
	}

	void sendFeatureMessage( const FeatureMessage& featureMessage );


private slots:
	void setScreenUpdateFlag()
	{
		m_screenUpdated = true;
	}

	void updateState();

private:
	const Computer& m_computer;

	Mode m_mode;
	State m_state;

	QSize m_screenSize;

	ItalcVncConnection* m_vncConnection;
	ItalcCoreConnection* m_coreConnection;

	bool m_screenUpdated;

};

typedef QList<ComputerControlInterface *> ComputerControlInterfaceList;

#endif // COMPUTER_CONTROL_INTERFACE_H
