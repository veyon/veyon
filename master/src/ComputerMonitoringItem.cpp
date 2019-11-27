/*
 * ComputerMonitoringItem.cpp - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "ComputerControlListModel.h"
#include "ComputerMonitoringItem.h"
#include "ComputerMonitoringModel.h"
#include "VeyonMaster.h"
#include "FeatureManager.h"
#include "VeyonConfiguration.h"


ComputerMonitoringItem::ComputerMonitoringItem( QQuickItem* parent ) :
	QQuickItem( parent )
{
}



ComputerControlInterfaceList ComputerMonitoringItem::selectedComputerControlInterfaces() const
{
	const auto& computerControlListModel = master()->computerControlListModel();
	ComputerControlInterfaceList computerControlInterfaces;

	computerControlInterfaces.reserve( m_selectedObjects.size() );

	for( const auto& selectedObject : m_selectedObjects )
	{
		computerControlInterfaces.append( computerControlListModel.computerControlInterface( selectedObject ) );
	}

	return computerControlInterfaces;
}



void ComputerMonitoringItem::componentComplete()
{
	initializeView();

	if( VeyonCore::config().autoAdjustGridSize() )
	{
		autoAdjustComputerScreenSize();
	}

	QQuickItem::componentComplete();
}



void ComputerMonitoringItem::autoAdjustComputerScreenSize()
{
}



void ComputerMonitoringItem::setUseCustomComputerPositions( bool enabled )
{
}



void ComputerMonitoringItem::alignComputers()
{
}



void ComputerMonitoringItem::runFeature( QString uid )
{
	ComputerMonitoringView::runFeature( master()->featureManager().feature( Feature::Uid( uid ) ) );
}



QObject* ComputerMonitoringItem::model() const
{
	return listModel();
}



QColor ComputerMonitoringItem::backgroundColor() const
{
	return m_backgroundColor;
}



QColor ComputerMonitoringItem::textColor() const
{
	return m_textColor;
}



const QSize& ComputerMonitoringItem::iconSize() const
{
	return m_iconSize;
}



void ComputerMonitoringItem::setColors( const QColor& backgroundColor, const QColor& textColor )
{
	m_backgroundColor = backgroundColor;
	m_textColor = textColor;

	emit backgroundColorChanged();
	emit textColorChanged();
}



QJsonArray ComputerMonitoringItem::saveComputerPositions()
{
	return {};
}



bool ComputerMonitoringItem::useCustomComputerPositions()
{
	return false;
}



void ComputerMonitoringItem::loadComputerPositions( const QJsonArray& positions )
{
}



void ComputerMonitoringItem::setIconSize( const QSize& size )
{
	if( size != m_iconSize )
	{
		m_iconSize = size;

		emit iconSizeChanged();
	}
}



void ComputerMonitoringItem::setSelectedObjects( const QVariantList& objects )
{
	m_selectedObjects.clear();

	for( const auto& object : objects )
	{
		const auto uuid = object.toUuid();
		if( uuid.isNull() == false )
		{
			m_selectedObjects.append( uuid );
		}
	}
}
