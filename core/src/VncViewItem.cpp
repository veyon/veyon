/*
 * VncViewItem.cpp - QtQuick VNC view item
 *
 * Copyright (c) 2019-2020 Tobias Junghans <tobydox@veyon.io>
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
#include "VeyonConnection.h"
#include "VncViewItem.h"


VncViewItem::VncViewItem( ComputerControlInterface::Pointer computerControlInterface, QQuickItem* parent ) :
	QQuickItem( parent ),
	VncView( computerControlInterface->connection()->vncConnection() ),
	m_computerControlInterface( computerControlInterface ),
	m_previousUpdateMode( m_computerControlInterface->updateMode() )
{
	connectUpdateFunctions( this );

	m_computerControlInterface->setUpdateMode( ComputerControlInterface::UpdateMode::Live );

	setAcceptHoverEvents( true );
	setAcceptedMouseButtons( Qt::AllButtons );
	setKeepMouseGrab( true );

	setFlag( ItemHasContents );
	setFlag( ItemIsFocusScope );
}



VncViewItem::~VncViewItem()
{
	m_computerControlInterface->setUpdateMode( m_previousUpdateMode );
}



QSGNode* VncViewItem::updatePaintNode( QSGNode* oldNode, UpdatePaintNodeData* updatePaintNodeData )
{
	Q_UNUSED(updatePaintNodeData)

	auto* node = static_cast<QSGSimpleTextureNode *>(oldNode);
	if( !node )
	{
		node = new QSGSimpleTextureNode();
		auto texture = new QSGImageTexture();
		node->setTexture( texture );
	}

	const auto texture = dynamic_cast<QSGImageTexture *>( node->texture() );

	if( viewport().isValid() )
	{
		texture->setImage( m_computerControlInterface->screen().copy( viewport() ) );
	}
	else
	{
		texture->setImage( m_computerControlInterface->screen() );
	}

	node->setRect( boundingRect() );

	return node;
}



void VncViewItem::setViewCursor( const QCursor& cursor )
{
	setCursor( cursor );
}



QSize VncViewItem::viewSize() const
{
	return { int(width()), int(height()) };
}



void VncViewItem::updateView( int x, int y, int w, int h )
{
	Q_UNUSED(x)
	Q_UNUSED(y)
	Q_UNUSED(w)
	Q_UNUSED(h)

	update();
}



bool VncViewItem::event( QEvent* event )
{
	return handleEvent( event ) || QQuickItem::event( event );
}
