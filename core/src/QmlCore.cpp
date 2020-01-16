/*
 *  QmlCore.cpp - QML-related core functions
 *
 *  Copyright (c) 2019-2020 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

#include "QmlCore.h"

QmlCore::QmlCore( QObject* parent ) :
	QObject( parent )
{
}



QObject* QmlCore::createObjectFromFile( const QString& qmlFile, QObject* parent, QObject* context )
{
	QQmlComponent component( qmlEngine(parent), qmlFile );

	auto object = component.beginCreate( qmlContext(parent) );
	object->setParent( parent );

	qmlEngine(object)->rootContext()->setContextProperty( QStringLiteral("qmlCore"), this );

	if( context )
	{
		qmlEngine(object)->rootContext()->setContextProperty( QStringLiteral("context"), context );
	}

	component.completeCreate();

	return object;
}



QQuickItem* QmlCore::createItemFromFile( const QString& qmlFile, QQuickItem* parent, QObject* context )
{
	QQmlComponent component( qmlEngine(parent), qmlFile );

	auto item = qobject_cast<QQuickItem *>( component.beginCreate( qmlContext(parent) ) );
	item->setParent( parent );
	item->setParentItem( parent );

	qmlEngine(item)->rootContext()->setContextProperty( QStringLiteral("qmlCore"), this );

	if( context )
	{
		qmlEngine(item)->rootContext()->setContextProperty( QStringLiteral("context"), context );
	}

	component.completeCreate();

	return item;
}



void QmlCore::deleteLater( QObject* object )
{
	object->deleteLater();
}
