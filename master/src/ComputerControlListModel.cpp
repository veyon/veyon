/*
 * ComputerControlListModel.cpp - data model for computer control objects
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

#include <QPainter>

#include "ComputerControlListModel.h"
#include "ComputerManager.h"
#include "FeatureManager.h"
#include "PlatformSessionFunctions.h"
#include "VeyonMaster.h"
#include "UserConfig.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#endif


ComputerControlListModel::ComputerControlListModel( VeyonMaster* masterCore, QObject* parent ) :
	ComputerListModel( parent ),
	m_master( masterCore ),
	m_iconHostOffline(QStringLiteral(":/master/host-offline.png")),
	m_iconHostOnline(QStringLiteral(":/master/host-online.png")),
	m_iconHostNameReslutionFailed(QStringLiteral(":/master/host-dns-error.png")),
	m_iconHostAccessDenied(QStringLiteral(":/master/host-access-denied.png")),
	m_iconHostServiceError(QStringLiteral(":/master/host-service-error.png"))
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
		return {};
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
		return uidRoleData(computerControl);

	case StateRole:
		return QVariant::fromValue( computerControl->state() );

	case UserLoginNameRole:
		return computerControl->userLoginName();

	case FramebufferRole:
		return computerControl->framebuffer();

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
		controlInterface->setScaledFramebufferSize( newSize );
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

	for( const auto& computer : computerList )
	{
		const auto controlInterface = ComputerControlInterface::Pointer::create( computer );
		m_computerControlInterfaces.append( controlInterface );
		startComputerControlInterface( controlInterface.data() );
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



QVariant ComputerControlListModel::uidRoleData(const ComputerControlInterface::Pointer& controlInterface) const
{
	static const QUuid uidRoleNamespace(QStringLiteral("7480c1c0-2b9f-46f1-92a6-0978bbfcd191"));

	switch (uidRoleContent())
	{
	case UidRoleContent::NetworkObjectUid:
		return controlInterface->computer().networkObjectUid();
	case UidRoleContent::SessionMetaDataHash:
		if (controlInterface->sessionInfo().metaData.isEmpty())
		{
			return controlInterface->computer().networkObjectUid();
		}
		return QUuid::createUuidV5(uidRoleNamespace, controlInterface->sessionInfo().metaData);
	}

	return {};
}



void ComputerControlListModel::updateState( const QModelIndex& index )
{
	Q_EMIT dataChanged( index, index, { Qt::DisplayRole, Qt::DecorationRole, Qt::ToolTipRole, FramebufferRole } );
}



void ComputerControlListModel::updateAccessControlMessage(const QModelIndex& index)
{
	Q_EMIT dataChanged(index, index, { Qt::ToolTipRole });
}



void ComputerControlListModel::updateScreen( const QModelIndex& index )
{
	Q_EMIT dataChanged( index, index, { Qt::DecorationRole, FramebufferRole } );
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



void ComputerControlListModel::updateSessionInfo(const QModelIndex& index)
{
	Q_EMIT dataChanged(index, index, {Qt::ToolTipRole});

	auto controlInterface = computerControlInterface( index );
	if (controlInterface.isNull() == false)
	{
		m_master->computerManager().updateSessionInfo(controlInterface);
	}
}



void ComputerControlListModel::startComputerControlInterface( ComputerControlInterface* controlInterface )
{
	controlInterface->start( computerScreenSize(), ComputerControlInterface::UpdateMode::Monitoring );

	connect( controlInterface, &ComputerControlInterface::framebufferSizeChanged,
			 this, &ComputerControlListModel::updateComputerScreenSize );

	connect( controlInterface, &ComputerControlInterface::framebufferUpdated,
			 this, [=] () { updateScreen( interfaceIndex( controlInterface ) ); } );

	connect( controlInterface, &ComputerControlInterface::activeFeaturesChanged,
			 this, [=] () { updateActiveFeatures( interfaceIndex( controlInterface ) ); } );

	connect( controlInterface, &ComputerControlInterface::stateChanged,
			 this, [=] () { updateState( interfaceIndex( controlInterface ) ); } );

	connect( controlInterface, &ComputerControlInterface::userChanged,
			 this, [=]() { updateUser( interfaceIndex( controlInterface ) ); } );

	connect(controlInterface, &ComputerControlInterface::sessionInfoChanged,
			 this, [=]() { updateSessionInfo(interfaceIndex(controlInterface)); });

	connect(controlInterface, &ComputerControlInterface::accessControlMessageChanged,
			this, [=] () { updateAccessControlMessage(interfaceIndex(controlInterface)); });
}



void ComputerControlListModel::stopComputerControlInterface( const ComputerControlInterface::Pointer& controlInterface )
{
	m_master->stopAllFeatures( { controlInterface } );

	controlInterface->disconnect(this);
	controlInterface->disconnect( &m_master->computerManager() );

	m_master->computerManager().clearOverlayModelData(controlInterface);
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
	const auto scaledIcon = icon.scaled(size.width(), size.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

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
		const auto image = controlInterface->scaledFramebuffer();
		if( image.isNull() == false )
		{
			return image;
		}

		return scaleAndAlignIcon(m_iconHostOnline, controlInterface->scaledFramebufferSize());
	}

	case ComputerControlInterface::State::HostNameResolutionFailed:
		return scaleAndAlignIcon(m_iconHostNameReslutionFailed, controlInterface->scaledFramebufferSize());

	case ComputerControlInterface::State::ServerNotRunning:
		return scaleAndAlignIcon(m_iconHostServiceError, controlInterface->scaledFramebufferSize());

	case ComputerControlInterface::State::AuthenticationFailed:
		return scaleAndAlignIcon(m_iconHostAccessDenied, controlInterface->scaledFramebufferSize());

	default:
		break;
	}

	if (controlInterface->accessControlMessage().isEmpty() == false)
	{
		return scaleAndAlignIcon(m_iconHostAccessDenied, controlInterface->scaledFramebufferSize());
	}

	return scaleAndAlignIcon(m_iconHostOffline, controlInterface->scaledFramebufferSize());
}



QString ComputerControlListModel::computerToolTipRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	const QString state( computerStateDescription( controlInterface ) );
	const QString displayName(tr("Name: %1").arg(controlInterface->computer().displayName()));
	const QString location( tr( "Location: %1" ).arg( controlInterface->computer().location() ) );
	const QString host =
			controlInterface->computer().hostAddress().isNull() ?
				tr("Hostname: %1").arg(controlInterface->computer().hostName().isEmpty() ?
										   QStringLiteral("&lt;%1&gt;").arg(tr("unknown"))
										 :
										   controlInterface->computer().hostName())
			  :
				tr("IP address: %1").arg(controlInterface->computer().hostAddress().toString());
	const QString user( userInformation( controlInterface ) );
	const QString features( tr( "Active features: %1" ).arg( activeFeatures( controlInterface ) ) );

	if( user.isEmpty() )
	{
		return QStringLiteral("<b>%1</b><br>%2<br>%3<br>%4<br>%5").arg(state, displayName, location, host, features);
	}

	return QStringLiteral("<b>%1</b><br>%2<br>%3<br>%4<br>%5<br>%6").arg(state, displayName, location, host, features, user);
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

		return QStringLiteral("%1 - %2").arg(user, controlInterface->computer().displayName());
	}

	if( displayRoleContent() != DisplayRoleContent::UserName )
	{
		return controlInterface->computer().displayName();
	}

	return tr("[no user]");
}



QString ComputerControlListModel::computerSortRole( const ComputerControlInterface::Pointer& controlInterface ) const
{
	switch( sortOrder() )
	{
	case SortOrder::ComputerAndUserName:
		return controlInterface->computer().location() + controlInterface->computer().displayName() +
				controlInterface->computer().hostName() + controlInterface->userLoginName();

	case SortOrder::ComputerName:
		return controlInterface->computer().location() + controlInterface->computer().displayName() +
				controlInterface->computer().hostName();

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

	case ComputerControlInterface::State::HostNameResolutionFailed:
		return tr("Hostname could not be resolved");

	case ComputerControlInterface::State::ServerNotRunning:
		return tr( "Veyon Server unreachable or not running" );

	case ComputerControlInterface::State::AuthenticationFailed:
		return tr( "Authentication failed or access denied" );

	default:
		break;
	}

	if (controlInterface->accessControlMessage().isEmpty() == false)
	{
		return controlInterface->accessControlMessage();
	}

	return tr( "Disconnected" );
}



QString ComputerControlListModel::userInformation(const ComputerControlInterface::Pointer& controlInterface)
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
			user = QStringLiteral("%1 (%2)").arg(controlInterface->userFullName(), user);
		}

		return tr( "Logged on user: %1" ).arg( user );
	}

	return {};
}



QString ComputerControlListModel::activeFeatures( const ComputerControlInterface::Pointer& controlInterface ) const
{
	QStringList featureNames;
	featureNames.reserve( controlInterface->activeFeatures().size() );

	for( const auto& feature : VeyonCore::featureManager().features() )
	{
		if( feature.testFlag( Feature::Flag::Master ) &&
			controlInterface->activeFeatures().contains( feature.uid() ) )
		{
			featureNames.append( feature.displayName() );
		}
	}

	featureNames.removeAll( {} );

	if( featureNames.isEmpty() )
	{
		return tr("[none]");
	}

	return featureNames.join( QStringLiteral(", ") );
}
