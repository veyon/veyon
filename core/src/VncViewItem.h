/*
 * VncViewItem.h - QtQuick VNC view item
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

#pragma once

#include <QQuickPaintedItem>

#include "ComputerControlInterface.h"
#include "VncView.h"

class VEYON_CORE_EXPORT VncViewItem : public QQuickItem, public VncView
{
	Q_OBJECT
public:
	VncViewItem( ComputerControlInterface::Pointer computerControlInterface, QQuickItem* parent = nullptr );
	~VncViewItem() override;

	QSGNode* updatePaintNode( QSGNode *oldNode, UpdatePaintNodeData* updatePaintNodeData ) override;

protected:
	virtual void updateView( int x, int y, int w, int h ) override;
	virtual QSize viewSize() const override;
	virtual void setViewCursor( const QCursor& cursor ) override;

	bool event( QEvent* event ) override;

private:
	ComputerControlInterface::Pointer m_computerControlInterface;
	ComputerControlInterface::UpdateMode m_previousUpdateMode;
	QSize m_framebufferSize;

};
