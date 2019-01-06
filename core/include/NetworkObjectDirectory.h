/*
 * NetworkObjectDirectory.h - base class for network object directory implementations
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

#ifndef NETWORK_OBJECT_DIRECTORY_H
#define NETWORK_OBJECT_DIRECTORY_H

#include <QObject>

#include "NetworkObject.h"

class QTimer;

class VEYON_CORE_EXPORT NetworkObjectDirectory : public QObject
{
	Q_OBJECT
public:
	enum {
		MinimumUpdateInterval = 10,
		DefaultUpdateInterval = 60,
		MaximumUpdateInterval = 3600
	};

	NetworkObjectDirectory( QObject* parent );

	void setUpdateInterval( int interval );

	virtual QList<NetworkObject> objects( const NetworkObject& parent ) = 0;

	virtual QList<NetworkObject> queryObjects( NetworkObject::Type type, const QString& name = QString() ) = 0;
	virtual NetworkObject queryParent( const NetworkObject& object ) = 0;

public slots:
	virtual void update() = 0;

private:
	QTimer* m_updateTimer;

signals:
	void objectsAboutToBeInserted( const NetworkObject& parent, int index, int count );
	void objectsInserted();
	void objectsAboutToBeRemoved( const NetworkObject& parent, int index, int count );
	void objectsRemoved();
	void objectChanged( const NetworkObject& parent, int index );

};

#endif // NETWORK_OBJECT_DIRECTORY_H
