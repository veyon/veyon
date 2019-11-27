/*
 * VncViewItem.cpp - QtQuick VNC view item
 *
 * Copyright (c) 2019 Tobias Junghans <tobydox@veyon.io>
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


#include <QSGSimpleTextureNode>

#include "QSGImageTexture.h"
#include "VncViewItem.h"


VncViewItem::VncViewItem( ComputerControlInterface::Pointer computerControlInterface, QQuickItem* parent ) :
	QQuickItem( parent ),
	m_computerControlInterface( computerControlInterface ),
	m_previousUpdateMode( m_computerControlInterface->updateMode() )
{
	connect( m_computerControlInterface.data(), &ComputerControlInterface::screenUpdated,
			 this, &VncViewItem::update );

	m_computerControlInterface->setUpdateMode( ComputerControlInterface::UpdateMode::Live );

	setFlag( ItemHasContents );
}



VncViewItem::~VncViewItem()
{
	m_computerControlInterface->setUpdateMode( m_previousUpdateMode );
}



QSGNode* VncViewItem::updatePaintNode( QSGNode* oldNode, UpdatePaintNodeData* updatePaintNodeData )
{
	Q_UNUSED(updatePaintNodeData)

	QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);
	if( !node )
	{
		node = new QSGSimpleTextureNode();
		auto texture = new QSGImageTexture();
		node->setTexture( texture );
	}

	dynamic_cast<QSGImageTexture *>( node->texture() )->setImage(m_computerControlInterface->screen() );
	node->setRect( boundingRect() );

	return node;
}
