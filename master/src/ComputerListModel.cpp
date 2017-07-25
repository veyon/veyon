/*
 * ComputerListModel.cpp - data model for computer objects
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QPainter>

#include "ComputerListModel.h"
#include "ComputerManager.h"




ComputerListModel::ComputerListModel(ComputerManager& manager, QObject *parent) :
	QAbstractListModel( parent ),
	m_dummyControlInterface( Computer() ),
	m_manager( manager ),
	m_iconDefault(),
	m_iconConnectionProblem(),
	m_iconDemoMode()
{
	connect( &m_manager, &ComputerManager::computerAboutToBeInserted,
			 this, &ComputerListModel::beginInsertComputer );
	connect( &m_manager, &ComputerManager::computerInserted,
			 this, &ComputerListModel::endInsertComputer );

	connect( &m_manager, &ComputerManager::computerAboutToBeRemoved,
			 this, &ComputerListModel::beginRemoveComputer );
	connect( &m_manager, &ComputerManager::computerRemoved,
			 this, &ComputerListModel::endRemoveComputer );

	connect( &m_manager, &ComputerManager::computerListAboutToBeReset,
			 this, &ComputerListModel::beginResetModel );
	connect( &m_manager, &ComputerManager::computerListAboutToBeReset,
			 this, &ComputerListModel::endResetModel );

	connect( &m_manager, &ComputerManager::computerScreenUpdated,
			 this, &ComputerListModel::updateComputerScreen );

	loadIcons();
}



int ComputerListModel::rowCount(const QModelIndex &parent) const
{
	if( parent.isValid() )
	{
		return 0;
	}

	return m_manager.computerList().count();
}



QVariant ComputerListModel::data(const QModelIndex &index, int role) const
{
	if( index.isValid() == false )
	{
		return QVariant();
	}

	if( index.row() >= m_manager.computerList().count() )
	{
		qCritical( "ComputerListModel::data(): index out of range!" );
	}

	const auto& computer = m_manager.computerList()[index.row()];

	switch( role )
	{
	case Qt::DecorationRole:
		return computerDecorationRole( computer );

	case Qt::ToolTipRole:
		return computerToolTipRole( computer );

	case Qt::DisplayRole:
		return computerDisplayRole( computer );

	default:
		break;
	}

	return QVariant();
}



ComputerControlInterface& ComputerListModel::computerControlInterface(const QModelIndex &index)
{
	if( index.isValid() == false || index.row() >= m_manager.computerList().count( ) )
	{
		qCritical( "ComputerListModel::computerControlInterface(): invalid ComputerControlInterface requested!" );
		return m_dummyControlInterface;
	}

	return m_manager.computerList()[index.row()].controlInterface();
}



void ComputerListModel::beginInsertComputer(int index)
{
	beginInsertRows( QModelIndex(), index, index );
}



void ComputerListModel::endInsertComputer()
{
	endInsertRows();
}



void ComputerListModel::beginRemoveComputer(int index)
{
	beginRemoveRows( QModelIndex(), index, index );
}



void ComputerListModel::endRemoveComputer()
{
	endRemoveRows();
}



void ComputerListModel::reload()
{
	beginResetModel();
	endResetModel();
}



void ComputerListModel::updateComputerScreen( int computerIndex )
{
	emit dataChanged( index( computerIndex, 0 ),
					  index( computerIndex, 0 ),
					  QVector<int>( { Qt::DisplayRole, Qt::DecorationRole, Qt::ToolTipRole } ) );
}



void ComputerListModel::loadIcons()
{
	m_iconDefault = prepareIcon( QImage( QStringLiteral(":/resources/preferences-desktop-display-gray.png") ) );
	m_iconConnectionProblem = prepareIcon( QImage( QStringLiteral(":/resources/preferences-desktop-display-red.png") ) );
	m_iconDemoMode = prepareIcon( QImage( QStringLiteral(":/resources/preferences-desktop-display-orange.png") ) );
}



QImage ComputerListModel::prepareIcon(const QImage &icon)
{
	QImage wideIcon( icon.width() * 16 / 9, icon.height(), QImage::Format_ARGB32 );
	wideIcon.fill( Qt::transparent );
	QPainter p( &wideIcon );
	p.drawImage( ( wideIcon.width() - icon.width() ) / 2, 0, icon );
	return wideIcon;
}



QImage ComputerListModel::computerDecorationRole( const Computer& computer ) const
{
	const auto& controlInterface = computer.controlInterface();

	QImage image;

	switch( controlInterface.state() )
	{
	case ComputerControlInterface::Connected:
		image = controlInterface.scaledScreen();
		if( image.isNull() == false )
		{
			return image;
		}

		image = m_iconDefault;
		break;

	case ComputerControlInterface::AuthenticationFailed:
	case ComputerControlInterface::ServiceUnreachable:
		image = m_iconConnectionProblem;
		break;

	default:
		image = m_iconDefault;
		break;
	}

	return image.scaled( controlInterface.scaledScreenSize(), Qt::KeepAspectRatio );
}



QString ComputerListModel::computerToolTipRole( const Computer& computer ) const
{
	const QString state( computerStateDescription( computer ) );
	const QString room( tr( "Room: %1" ).arg( computer.room() ) );
	const QString host( tr( "Host/IP address: %1" ).arg( computer.hostAddress() ) );
	const QString user( loggedOnUserInformation( computer ) );

	if( user.isEmpty() )
	{
		return QStringLiteral( "<b>%1</b><br>%2<br>%3" ).arg( state, room, host );
	}

	return QStringLiteral( "<b>%1</b><br>%2<br>%3<br>%4" ).arg( state, room, host, user );
}



QString ComputerListModel::computerDisplayRole( const Computer& computer ) const
{
	const auto& controlInterface = computer.controlInterface();

	if( controlInterface.state() == ComputerControlInterface::Connected &&
			controlInterface.user().isEmpty() == false )
	{
		auto user = controlInterface.user();

		// do we have full name information?
		QRegExp fullNameRX( "(.*) \\((.*)\\)" );
		if( fullNameRX.indexIn( user ) >= 0 )
		{
			if( fullNameRX.cap( 2 ).isEmpty() == false )
			{
				user = fullNameRX.cap( 2 );
			}
			else
			{
				user = fullNameRX.cap( 1 );
			}
		}

		return QStringLiteral("%1 - %2").arg( user, computer.name() );
	}

	return computer.name();
}



QString ComputerListModel::computerStateDescription( const Computer& computer ) const
{
	switch( computer.controlInterface().state() )
	{
	case ComputerControlInterface::Connected:
		return tr( "Online and connected" );

	case ComputerControlInterface::Connecting:
		return tr( "Establishing connection" );

	case ComputerControlInterface::Offline:
		return tr( "Computer offline or switched off" );

	case ComputerControlInterface::ServiceUnreachable:
		return tr( "Service unreachable or not running" );

	case ComputerControlInterface::AuthenticationFailed:
		return tr( "Authentication failed or access denied" );

	default:
		break;
	}

	return tr( "Disconnected" );
}



QString ComputerListModel::loggedOnUserInformation( const Computer& computer ) const
{
	const auto& controlInterface = computer.controlInterface();

	if( controlInterface.state() == ComputerControlInterface::Connected )
	{
		if( controlInterface.user().isEmpty() )
		{
			return tr( "No user logged on" );
		}

		return tr( "Logged on user: %1" ).arg( controlInterface.user() );
	}

	return QString();
}
