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
	m_iconUnknownState(),
	m_iconComputerUnreachable(),
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

	connect( &m_manager, &ComputerManager::computerScreenSizeChanged,
			 this, &ComputerListModel::updateComputerScreenSize );

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

	if( role == Qt::DecorationRole )
	{
		return computerDecorationRole( m_manager.computerList()[index.row()].controlInterface() );
	}

	if( role != Qt::DisplayRole)
	{
		return QVariant();
	}

	return m_manager.computerList()[index.row()].name();
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
					  QVector<int>( { Qt::DecorationRole } ) );
}



void ComputerListModel::updateComputerScreenSize()
{
	emit layoutChanged();
}



void ComputerListModel::loadIcons()
{
	m_iconUnknownState = prepareIcon( QImage( ":/resources/preferences-desktop-display-gray.png" ) );
	m_iconComputerUnreachable = prepareIcon( QImage( ":/resources/preferences-desktop-display-red.png" ) );
	m_iconDemoMode = prepareIcon( QImage( ":/resources/preferences-desktop-display-orange.png" ) );
}



QImage ComputerListModel::prepareIcon(const QImage &icon)
{
	QImage wideIcon( icon.width() * 16 / 9, icon.height(), QImage::Format_ARGB32 );
	wideIcon.fill( Qt::transparent );
	QPainter p( &wideIcon );
	p.drawImage( ( wideIcon.width() - icon.width() ) / 2, 0, icon );
	return wideIcon;
}



QImage ComputerListModel::computerDecorationRole( const ComputerControlInterface &controlInterface ) const
{
	QImage image;

	switch( controlInterface.state() )
	{
	case ComputerControlInterface::Connected:
		image = controlInterface.scaledScreen();
		if( image.isNull() == false )
		{
			return image;
		}

		image = m_iconUnknownState;
		break;

	case ComputerControlInterface::Unreachable:
		image = m_iconComputerUnreachable;
		break;

	default:
		image = m_iconUnknownState;
		break;
	}

	return image.scaled( controlInterface.scaledScreenSize(), Qt::KeepAspectRatio );
}
