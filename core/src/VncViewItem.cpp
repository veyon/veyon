/*
 * VncViewItem.cpp - QtQuick VNC view item
 *
 * Copyright (c) 2019-2021 Tobias Junghans <tobydox@veyon.io>
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


VncViewItem::VncViewItem( const ComputerControlInterface::Pointer& computerControlInterface, QQuickItem* parent ) :
	QQuickItem( parent ),
	VncView( computerControlInterface )
{
	connectUpdateFunctions( this );

	setAcceptHoverEvents( true );
	setAcceptedMouseButtons( Qt::AllButtons );
	setKeepMouseGrab( true );

	setFlag( ItemHasContents );
	setFlag( ItemIsFocusScope );
}



VncViewItem::~VncViewItem()
{
	// do not receive any signals during connection shutdown
	connection()->disconnect( this );
}



QSGNode* VncViewItem::updatePaintNode( QSGNode* oldNode, UpdatePaintNodeData* updatePaintNodeData )
{
	Q_UNUSED(updatePaintNodeData)

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	// TODO
	return oldNode;
#else
	auto* node = static_cast<QSGSimpleTextureNode *>(oldNode);
	if( !node )
	{
		node = new QSGSimpleTextureNode();
		auto texture = new QSGImageTexture();
		node->setTexture( texture );
	}

	const auto texture = qobject_cast<QSGImageTexture *>( node->texture() );

	if( viewport().isValid() )
	{
		texture->setImage( computerControlInterface()->screen().copy( viewport() ) );
	}
	else
	{
		texture->setImage( computerControlInterface()->screen() );
	}
	node->setRect( boundingRect() );

	return node;
#endif
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
