/*
 * TextMessageFeature.cpp - implementation of TextMessageFeature class
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

#include "TextMessageFeature.h"
#include "TextMessageDialog.h"
#include "ComputerControlInterface.h"
#include "ItalcCoreConnection.h"


TextMessageFeature::TextMessageFeature() :
	m_features()
{
	m_features += Feature( Feature::Action,
						   Feature::ScopeAll,
						   Feature::Uid( "e75ae9c8-ac17-4d00-8f0d-019348346208" ),
						   tr( "Text message" ), QString(),
						   tr( "Use this function to send a text message to all "
							   "users e.g. to assign them new tasks." ),
						   QIcon( ":/dialog-information.png" ) );
}



const FeatureList &TextMessageFeature::featureList() const
{
	return m_features;
}



bool TextMessageFeature::runMasterFeature( const Feature& feature,
										   const ComputerControlInterfaceList& computerControlInterfaces,
										   QWidget* parent )
{
	if( m_features.contains( feature ) == false )
	{
		return false;
	}

	QString textMessage;

	TextMessageDialog( textMessage, qobject_cast<QWidget *>( parent ) ).exec();

	if( textMessage.isEmpty() )
	{
		for( auto interface : computerControlInterfaces )
		{
			if( interface->coreConnection() )
			{
				interface->coreConnection()->displayTextMessage( tr( "Message from teacher" ), textMessage );
			}
		}
	}

	return true;
}



bool TextMessageFeature::runServiceFeature( const Feature& feature, SocketDevice& socketDevice, const ItalcCore::Msg& message )
{
}
