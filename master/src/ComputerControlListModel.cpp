/*
 * ComputerControlListModel.cpp - data model for computer control objects
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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
#include <QTimer>

#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "FeatureManager.h"
#include "VeyonMaster.h"
#include "UserConfig.h"
#include "VeyonConfiguration.h"


ComputerControlListModel::ComputerControlListModel( VeyonMaster* masterCore, QObject* parent ) :
	QAbstractListModel( parent ),
	m_master( masterCore ),
	m_displayRoleContent( static_cast<DisplayRoleContent>( VeyonCore::config().computerDisplayRoleContent() ) ),
	m_iconDefault(),
	m_iconConnectionProblem(),
	m_iconDemoMode()
{
	loadIcons();

	connect( &m_master->computerManager(), &ComputerManager::computerSelectionReset,
			 this, &ComputerControlListModel::reload );
	connect( &m_master->computerManager(), &ComputerManager::computerSelectionChanged,
			 this, &ComputerControlListModel::update );

	auto computerScreenUpdateTimer = new QTimer( this );
	connect( computerScreenUpdateTimer, &QTimer::timeout, this, &ComputerControlListModel::updateComputerScreens );
	computerScreenUpdateTimer->start( VeyonCore::config().computerMonitoringUpdateInterval() );

	reload();
}



int ComputerControlListModel::rowCount( const QModelIndex& parent ) const
{
	if( parent.isValid() )
	{
		return 0;
	}

	return m_computerControlInterfaces.count();
}



QVariant ComputerControlListModel::data( const QModelIndex& index, int role ) const
{
	if( index.isValid() == false )
	{
		return QVariant();
	}

	if( index.row() >= m_computerControlInterfaces.count() )
	{
		qCritical( "ComputerListModel::data(): index out of range!" );
	}

	const auto computerControl = m_computerControlInterfaces[index.row()];

	switch( role )
	{
	case Qt::DecorationRole:
		return computerDecorationRole( computerControl );

	case Qt::ToolTipRole:
		return computerToolTipRole( computerControl );

	case Qt::DisplayRole:
		return computerDisplayRole( computerControl );

	case Qt::InitialSortOrderRole:
		return computerControl->computer().room() + computerControl->computer().name() +
				computerControl->computer().hostAddress() + computerControl->user();

	case UidRole:
		return computerControl->computer().networkObjectUid();

	case StateRole:
		return computerControl->state();

	default:
		break;
	}

	return QVariant();
}



void ComputerControlListModel::updateComputerScreenSize()
{
	const auto size = computerScreenSize();

	for( auto& controlInterface : m_computerControlInterfaces )
	{
		controlInterface->setScaledScreenSize( size );
	}
}



ComputerControlInterface::Pointer ComputerControlListModel::computerControlInterface( const QModelIndex& index ) const
{
	if( index.isValid() == false || index.row() >= m_computerControlInterfaces.count() )
	{
		qCritical( "ComputerListModel::computerControlInterface(): invalid ComputerControlInterface requested!" );
		return ComputerControlInterface::Pointer();
	}

	return m_computerControlInterfaces[index.row()];
}



void ComputerControlListModel::reload()
{
	beginResetModel();

	const auto computerList = m_master->computerManager().selectedComputers( QModelIndex() );

	m_computerControlInterfaces.clear();
	m_computerControlInterfaces.reserve( computerList.size() );

	int row = 0;

	for( const auto& computer : computerList )
	{
		const auto controlInterface = ComputerControlInterface::Pointer::create( computer );
		m_computerControlInterfaces.append( controlInterface );
		startComputerControlInterface( controlInterface, index( row ) );
		++row;
	}

	endResetModel();
}



void ComputerControlListModel::update()
{
	const auto newComputerList = m_master->computerManager().selectedComputers( QModelIndex() );

	int row = 0;

	for( auto it = m_computerControlInterfaces.begin(); it != m_computerControlInterfaces.end(); ) // clazy:exclude=detaching-member
	{
		if( newComputerList.contains( (*it)->computer() ) == false )
		{
			stopComputerControlInterface( *it );

			beginRemoveRows( QModelIndex(), row, row );
			it = m_computerControlInterfaces.erase( it );
			endRemoveRows();
		}
		else
		{
			++it;
			++row;
		}
	}

	row = 0;

	for( const auto& computer : newComputerList )
	{
		if( row < m_computerControlInterfaces.count() && m_computerControlInterfaces[row]->computer() != computer )
		{
			beginInsertRows( QModelIndex(), row, row );
			const auto controlInterface = ComputerControlInterface::Pointer::create( computer );
			m_computerControlInterfaces.insert( row, controlInterface );
			startComputerControlInterface( controlInterface, index( row ) );
			endInsertRows();
		}
		else if( row >= m_computerControlInterfaces.count() )
		{
			beginInsertRows( QModelIndex(), row, row );
			const auto controlInterface = ComputerControlInterface::Pointer::create( computer );
			m_computerControlInterfaces.append( controlInterface );
			startComputerControlInterface( controlInterface, index( row ) ); // clazy:exclude=detaching-member
			endInsertRows();
		}

		++row;
	}
}



void ComputerControlListModel::updateComputerScreens()
{
	int computerIndex = 0;

	for( auto& controlInterface : m_computerControlInterfaces )
	{
		if( controlInterface->hasScreenUpdates() )
		{
			controlInterface->clearScreenUpdateFlag();

			emit dataChanged( index( computerIndex, 0 ),
							  index( computerIndex, 0 ),
							  QVector<int>( { Qt::DisplayRole, Qt::DecorationRole, Qt::ToolTipRole } ) );
		}

		++computerIndex;
	}
}



void ComputerControlListModel::startComputerControlInterface( const ComputerControlInterface::Pointer& controlInterface,
															  const QModelIndex& index )
{
	controlInterface->start( computerScreenSize(), &m_master->builtinFeatures() );

	connect( controlInterface.data(), &ComputerControlInterface::featureMessageReceived, this,
			 [=]( const FeatureMessage& featureMessage, ComputerControlInterface::Pointer computerControlInterface ) {
		m_master->featureManager().handleFeatureMessage( *m_master, featureMessage, computerControlInterface );
	} );

	connect( controlInterface.data(), &ComputerControlInterface::activeFeaturesChanged,
			 this, [=] () { emit activeFeaturesChanged( index ); } );

	// pass weak pointer to lambda function as otherwise the original shared pointer
	// gets referenced once more all the time and thus the object never gets deleted
	auto controlInterfaceWeakRef = controlInterface->weakPointer();
	connect( controlInterface.data(), &ComputerControlInterface::userChanged,
			 &m_master->computerManager(),
			 [=] () { m_master->computerManager().updateUser( controlInterfaceWeakRef ); } );
}



void ComputerControlListModel::stopComputerControlInterface( const ComputerControlInterface::Pointer& controlInterface )
{
	m_master->stopAllModeFeatures( { controlInterface } );

	controlInterface->disconnect( &m_master->computerManager() );

	controlInterface->setUser( QString() );
	m_master->computerManager().updateUser( controlInterface );
}



QSize ComputerControlListModel::computerScreenSize() const
{
	return QSize( m_master->userConfig().monitoringScreenSize(),
				  m_master->userConfig().monitoringScreenSize() * 9 / 16 );
}



void ComputerControlListModel::loadIcons()
{
	m_iconDefault = prepareIcon( QImage( QStringLiteral(":/resources/preferences-desktop-display-gray.png") ) );
	m_iconConnectionProblem = prepareIcon( QImage( QStringLiteral(":/resources/preferences-desktop-display-red.png") ) );
	m_iconDemoMode = prepareIcon( QImage( QStringLiteral(":/resources/preferences-desktop-display-orange.png") ) );
}



QImage ComputerControlListModel::prepareIcon(const QImage &icon)
{
	QImage wideIcon( icon.width() * 16 / 9, icon.height(), QImage::Format_ARGB32 );
	wideIcon.fill( Qt::transparent );
	QPainter p( &wideIcon );
	p.drawImage( ( wideIcon.width() - icon.width() ) / 2, 0, icon );
	return wideIcon;
}



QImage ComputerControlListModel::computerDecorationRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	QImage image;

	switch( controlInterface->state() )
	{
	case ComputerControlInterface::Connected:
		image = controlInterface->scaledScreen();
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

	return image.scaled( controlInterface->scaledScreenSize(), Qt::KeepAspectRatio );
}



QString ComputerControlListModel::computerToolTipRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	const QString state( computerStateDescription( controlInterface ) );
	const QString room( tr( "Room: %1" ).arg( controlInterface->computer().room() ) );
	const QString host( tr( "Host/IP address: %1" ).arg( controlInterface->computer().hostAddress() ) );
	const QString user( loggedOnUserInformation( controlInterface ) );
	const QString features( tr( "Active features: %1" ).arg( activeFeatures( controlInterface ) ) );

	if( user.isEmpty() )
	{
		return QStringLiteral( "<b>%1</b><br>%2<br>%3<br>%4" ).arg( state, room, host, features );
	}

	return QStringLiteral( "<b>%1</b><br>%2<br>%3<br>%4<br>%5" ).arg( state, room, host, features, user);
}



QString ComputerControlListModel::computerDisplayRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	if( m_displayRoleContent != DisplayComputerName &&
			controlInterface->state() == ComputerControlInterface::Connected &&
			controlInterface->user().isEmpty() == false )
	{
		auto user = controlInterface->user();

		// do we have full name information?
		QRegExp fullNameRX( QStringLiteral("(.*) \\((.*)\\)") );
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

		if( m_displayRoleContent == DisplayUserName )
		{
			return user;
		}
		else
		{
			return QStringLiteral("%1 - %2").arg( user, controlInterface->computer().name() );
		}
	}

	if( m_displayRoleContent != DisplayUserName )
	{
		return controlInterface->computer().name();
	}

	return QString();
}



QString ComputerControlListModel::computerStateDescription( const ComputerControlInterface::Pointer& controlInterface )
{
	switch( controlInterface->state() )
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



QString ComputerControlListModel::loggedOnUserInformation( const ComputerControlInterface::Pointer& controlInterface )
{
	if( controlInterface->state() == ComputerControlInterface::Connected )
	{
		if( controlInterface->user().isEmpty() )
		{
			return tr( "No user logged on" );
		}

		return tr( "Logged on user: %1" ).arg( controlInterface->user() );
	}

	return QString();
}



QString ComputerControlListModel::activeFeatures( const ComputerControlInterface::Pointer& controlInterface ) const
{
	QStringList featureNames;

	for( const auto& feature : m_master->features() )
	{
		if( controlInterface->activeFeatures().contains( feature.uid().toString() ) )
		{
			featureNames.append( feature.displayName() );
		}
	}

	featureNames.removeAll( QString() );

	return featureNames.join( QStringLiteral(", ") );
}



Qt::ItemFlags ComputerControlListModel::flags( const QModelIndex& index ) const
{
	auto defaultFlags = QAbstractListModel::flags( index );

	if( index.isValid() )
	{
		 return Qt::ItemIsDragEnabled | defaultFlags;
	}

	return Qt::ItemIsDropEnabled | defaultFlags;
}



Qt::DropActions ComputerControlListModel::supportedDragActions() const
{
	return Qt::MoveAction;
}



Qt::DropActions ComputerControlListModel::supportedDropActions() const
{
	return Qt::MoveAction;
}
