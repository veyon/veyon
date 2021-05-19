/*
 * ComputerControlListModel.cpp - data model for computer control objects
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

#include <QPainter>

#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "FeatureManager.h"
#include "VeyonMaster.h"
#include "UserConfig.h"
#include "VeyonConfiguration.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#endif


ComputerControlListModel::ComputerControlListModel( VeyonMaster* masterCore, QObject* parent ) :
	ComputerListModel( parent ),
	m_master( masterCore ),
	m_iconDefault( QStringLiteral(":/master/preferences-desktop-display-gray.png") ),
	m_iconConnectionProblem( QStringLiteral(":/master/preferences-desktop-display-red.png") ),
	m_iconServerNotRunning( QStringLiteral(":/master/preferences-desktop-display-orange.png") )
{
#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif

	connect( &m_master->computerManager(), &ComputerManager::computerSelectionReset,
			 this, &ComputerControlListModel::reload );
	connect( &m_master->computerManager(), &ComputerManager::computerSelectionChanged,
			 this, &ComputerControlListModel::update );

	updateComputerScreenSize();

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
		vCritical() << "index out of range!";
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
		return computerSortRole( computerControl );

	case UidRole:
		return computerControl->computer().networkObjectUid();

	case StateRole:
		return QVariant::fromValue( computerControl->state() );

	case ScreenRole:
		return computerControl->screen();

	case ControlInterfaceRole:
		return QVariant::fromValue( computerControl );

	default:
		break;
	}

	return {};
}



void ComputerControlListModel::updateComputerScreenSize()
{
	auto ratio = 16.0 / 9.0;

	switch( aspectRatio() )
	{
	case AspectRatio::Auto: ratio = averageAspectRatio(); break;
	case AspectRatio::AR16_9: ratio = 16.0 / 9.0; break;
	case AspectRatio::AR16_10: ratio = 16.0 / 10.0; break;
	case AspectRatio::AR3_2: ratio = 3.0 / 2.0; break;
	case AspectRatio::AR4_3: ratio = 4.0 / 3.0; break;

	}

	const QSize newSize{ m_master->userConfig().monitoringScreenSize(),
						 int(m_master->userConfig().monitoringScreenSize() / ratio) };

	for( auto& controlInterface : m_computerControlInterfaces )
	{
		controlInterface->setScaledScreenSize( newSize );
	}

	if( m_computerScreenSize != newSize )
	{
		m_computerScreenSize = newSize;

		for( int i = 0; i < rowCount(); ++i )
		{
			updateScreen( index( i ) );
		}

		Q_EMIT computerScreenSizeChanged();
	}
}



ComputerControlInterface::Pointer ComputerControlListModel::computerControlInterface( const QModelIndex& index ) const
{
	if( index.isValid() == false || index.row() >= m_computerControlInterfaces.count() )
	{
		vCritical() << "invalid ComputerControlInterface requested!";
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
		startComputerControlInterface( controlInterface.data() );
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
			startComputerControlInterface( controlInterface.data() );
			endInsertRows();
		}
		else if( row >= m_computerControlInterfaces.count() )
		{
			beginInsertRows( QModelIndex(), row, row );
			const auto controlInterface = ComputerControlInterface::Pointer::create( computer );
			m_computerControlInterfaces.append( controlInterface );
			startComputerControlInterface( controlInterface.data() );
			endInsertRows();
		}

		++row;
	}

	updateComputerScreenSize();
}



QModelIndex ComputerControlListModel::interfaceIndex( ComputerControlInterface* controlInterface ) const
{
	return ComputerListModel::index( m_computerControlInterfaces.indexOf( controlInterface->weakPointer() ), 0 );
}



void ComputerControlListModel::updateState( const QModelIndex& index )
{
	Q_EMIT dataChanged( index, index, { Qt::DisplayRole, Qt::DecorationRole, Qt::ToolTipRole, ScreenRole } );
}



void ComputerControlListModel::updateScreen( const QModelIndex& index )
{
	Q_EMIT dataChanged( index, index, { Qt::DecorationRole, ScreenRole } );
}



void ComputerControlListModel::updateActiveFeatures( const QModelIndex& index )
{
	Q_EMIT dataChanged( index, index, { Qt::ToolTipRole } );
	Q_EMIT activeFeaturesChanged( index );
}



void ComputerControlListModel::updateUser( const QModelIndex& index )
{
	Q_EMIT dataChanged( index, index, { Qt::DisplayRole, Qt::ToolTipRole } );

	auto controlInterface = computerControlInterface( index );
	if( controlInterface.isNull() == false )
	{
		m_master->computerManager().updateUser( controlInterface );
	}
}



void ComputerControlListModel::startComputerControlInterface( ComputerControlInterface* controlInterface )
{
	controlInterface->start( computerScreenSize(), ComputerControlInterface::UpdateMode::Monitoring );

	connect( controlInterface, &ComputerControlInterface::featureMessageReceived, this,
			 [=]( const FeatureMessage& featureMessage, const ComputerControlInterface::Pointer& computerControlInterface ) {
				 m_master->featureManager().handleFeatureMessage( computerControlInterface, featureMessage );
	} );

	connect( controlInterface, &ComputerControlInterface::screenSizeChanged,
			 this, &ComputerControlListModel::updateComputerScreenSize );

	connect( controlInterface, &ComputerControlInterface::screenUpdated,
			 this, [=] () { updateScreen( interfaceIndex( controlInterface ) ); } );

	connect( controlInterface, &ComputerControlInterface::activeFeaturesChanged,
			 this, [=] () { updateActiveFeatures( interfaceIndex( controlInterface ) ); } );

	connect( controlInterface, &ComputerControlInterface::stateChanged,
			 this, [=] () { updateState( interfaceIndex( controlInterface ) ); } );

	connect( controlInterface, &ComputerControlInterface::userChanged,
			 this, [=]() { updateUser( interfaceIndex( controlInterface ) ); } );
}



void ComputerControlListModel::stopComputerControlInterface( const ComputerControlInterface::Pointer& controlInterface )
{
	m_master->stopAllModeFeatures( { controlInterface } );

	controlInterface->disconnect( &m_master->computerManager() );

	controlInterface->setUserInformation( {}, {}, -1 );
	m_master->computerManager().updateUser( controlInterface );
}



double ComputerControlListModel::averageAspectRatio() const
{
	QSize size{ 16, 9 };

	for( const auto& controlInterface : m_computerControlInterfaces )
	{
		const auto currentSize = controlInterface->screenSize();
		if( currentSize.isValid() )
		{
			size += currentSize;
		}
	}

	return double(size.width()) / double(size.height());
}



QImage ComputerControlListModel::scaleAndAlignIcon( const QImage& icon, QSize size ) const
{
	const auto scaledIcon = icon.scaled( size.width(), size.height(), Qt::KeepAspectRatio );

	QImage scaledAndAlignedIcon( size, QImage::Format_ARGB32 );
	scaledAndAlignedIcon.fill( Qt::transparent );

	QPainter painter( &scaledAndAlignedIcon );
	painter.drawImage( ( scaledAndAlignedIcon.width() - scaledIcon.width() ) / 2,
					   ( scaledAndAlignedIcon.height() - scaledIcon.height() ) / 2,
					   scaledIcon );

	return scaledAndAlignedIcon;
}



QImage ComputerControlListModel::computerDecorationRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	switch( controlInterface->state() )
	{
	case ComputerControlInterface::State::Connected:
	{
		const auto image = controlInterface->scaledScreen();
		if( image.isNull() == false )
		{
			return image;
		}

		return scaleAndAlignIcon( m_iconDefault, controlInterface->scaledScreenSize() );
	}

	case ComputerControlInterface::State::ServerNotRunning:
		return scaleAndAlignIcon( m_iconServerNotRunning, controlInterface->scaledScreenSize() );

	case ComputerControlInterface::State::AuthenticationFailed:
		return scaleAndAlignIcon( m_iconConnectionProblem, controlInterface->scaledScreenSize() );

	default:
		break;
	}

	return scaleAndAlignIcon( m_iconDefault, controlInterface->scaledScreenSize() );
}



QString ComputerControlListModel::computerToolTipRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	const QString state( computerStateDescription( controlInterface ) );
	const QString location( tr( "Location: %1" ).arg( controlInterface->computer().location() ) );
	const QString host( tr( "Host/IP address: %1" ).arg( controlInterface->computer().hostAddress().isEmpty()
															 ? QStringLiteral("&lt;%1&gt;").arg( tr("invalid") )
															 : controlInterface->computer().hostAddress() ) );
	const QString user( loggedOnUserInformation( controlInterface ) );
	const QString features( tr( "Active features: %1" ).arg( activeFeatures( controlInterface ) ) );

	if( user.isEmpty() )
	{
		return QStringLiteral( "<b>%1</b><br>%2<br>%3<br>%4" ).arg( state, location, host, features );
	}

	return QStringLiteral( "<b>%1</b><br>%2<br>%3<br>%4<br>%5" ).arg( state, location, host, features, user);
}



QString ComputerControlListModel::computerDisplayRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	if( displayRoleContent() != DisplayRoleContent::ComputerName &&
		controlInterface->state() == ComputerControlInterface::State::Connected &&
		controlInterface->userLoginName().isEmpty() == false )
	{
		auto user = controlInterface->userFullName();
		if( user.isEmpty() )
		{
			user = controlInterface->userLoginName();
		}

		if( displayRoleContent() == DisplayRoleContent::UserName )
		{
			return user;
		}
		else
		{
			return QStringLiteral("%1 - %2").arg( user, controlInterface->computer().name() );
		}
	}

	if( displayRoleContent() != DisplayRoleContent::UserName )
	{
		return controlInterface->computer().name();
	}

	return tr("[no user]");
}



QString ComputerControlListModel::computerSortRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	switch( sortOrder() )
	{
	case SortOrder::ComputerAndUserName:
		return controlInterface->computer().location() + controlInterface->computer().name() +
				controlInterface->computer().hostAddress() + controlInterface->userLoginName();

	case SortOrder::ComputerName:
		return controlInterface->computer().location() + controlInterface->computer().name() +
				controlInterface->computer().hostAddress();

	case SortOrder::UserName:
		if( controlInterface->userFullName().isEmpty() == false )
		{
			return controlInterface->userFullName();
		}

		return controlInterface->userLoginName();
	}

	return {};
}



QString ComputerControlListModel::computerStateDescription( const ComputerControlInterface::Pointer& controlInterface )
{
	switch( controlInterface->state() )
	{
	case ComputerControlInterface::State::Connected:
		return tr( "Online and connected" );

	case ComputerControlInterface::State::Connecting:
		return tr( "Establishing connection" );

	case ComputerControlInterface::State::HostOffline:
		return tr( "Computer offline or switched off" );

	case ComputerControlInterface::State::ServerNotRunning:
		return tr( "Veyon Server unreachable or not running" );

	case ComputerControlInterface::State::AuthenticationFailed:
		return tr( "Authentication failed or access denied" );

	default:
		break;
	}

	return tr( "Disconnected" );
}



QString ComputerControlListModel::loggedOnUserInformation( const ComputerControlInterface::Pointer& controlInterface )
{
	if( controlInterface->state() == ComputerControlInterface::State::Connected )
	{
		if( controlInterface->userLoginName().isEmpty() )
		{
			return tr( "No user logged on" );
		}

		auto user = controlInterface->userLoginName();
		if( controlInterface->userFullName().isEmpty() == false )
		{
			user = QStringLiteral( "%1 (%2)" ).arg( user, controlInterface->userFullName() );
		}

		return tr( "Logged on user: %1" ).arg( user );
	}

	return {};
}



QString ComputerControlListModel::activeFeatures( const ComputerControlInterface::Pointer& controlInterface ) const
{
	QStringList featureNames;

	for( const auto& feature : m_master->features() )
	{
		if( controlInterface->activeFeatures().contains( feature.uid() ) )
		{
			featureNames.append( feature.displayName() );
		}
	}

	featureNames.removeAll( {} );

	return featureNames.join( QStringLiteral(", ") );
}
