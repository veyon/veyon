/*
 * ConfigurationXmlStore.cpp - implementation of XmlStore
 *
 * Copyright (c) 2009-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtXml/QDomDocument>

#include "Configuration/XmlStore.h"
#include "Configuration/Object.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "DecoratedMessageBox.h"


namespace Configuration
{

XmlStore::XmlStore( Scope _scope ) :
	Store( Store::XmlFile, _scope )
{
}




static void loadXmlTree( Object * _obj, QDomNode & _parentNode,
				const QString & _parentKey )
{
	QDomNode node = _parentNode.firstChild();

	while( !node.isNull() )
	{
		if( !node.firstChildElement().isNull() )
		{
			const QString subParentKey = _parentKey +
				( _parentKey.isEmpty() ? "" : "/" ) +
					node.nodeName();
			loadXmlTree( _obj, node, subParentKey );
		}
		else
		{
			_obj->setValue( node.nodeName(),
					node.toElement().text(),
					_parentKey );
		}
		node = node.nextSibling();
	}
}



void XmlStore::load( Object * _obj )
{
	QDomDocument doc;
	QFile xmlFile( configurationFilePath() );
	if( !xmlFile.open( QFile::ReadOnly ) || !doc.setContent( &xmlFile ) )
	{
		DecoratedMessageBox::information(
			_obj->tr( "No configuration file found" ),
			_obj->tr( "Could not open configuration file %1.\n"
				"You will have to add at least one classroom "
				"and computers using the classroom-manager which "
				"you'll find inside the program in the sidebar on "
				"the left side." ).arg( xmlFile.fileName() ) );
		return;
	}

	QDomElement root = doc.documentElement();
	loadXmlTree( _obj, root, QString() );
}




static void saveXmlTree( const Object::DataMap & _dataMap,
				QDomDocument & _doc,
				QDomNode & _parentNode,
				const QString & _parentKey )
{
	for( Object::DataMap::ConstIterator it = _dataMap.begin();
						it != _dataMap.end(); ++it )
	{
		if( it.value().type() == QVariant::Map )
		{
			// create a new element with current key as tagname
			QDomNode node = _doc.createElement( it.key() );

			const QString subParentKey = _parentKey +
				( _parentKey.isEmpty() ? "" : "/" ) + it.key();
			saveXmlTree( it.value().toMap(),
					_doc,
					node,
					subParentKey );
			_parentNode.appendChild( node );
		}
		else if( it.value().type() == QVariant::String )
		{
			QDomElement e = _doc.createElement( it.key() );
			QDomText t = _doc.createTextNode( it.value().toString() );
			e.appendChild( t );
			_parentNode.appendChild( e );
		}
	}
}




void XmlStore::flush( Object * _obj )
{
	QDomDocument doc( "ItalcConfigXmlStore" );
	const Object::DataMap & data = _obj->data();

	QDomElement root = doc.createElement( configurationNameFromScope() );
	saveXmlTree( data, doc, root, QString() );
	doc.appendChild( root );

	QFile outfile( configurationFilePath() );
	if( !outfile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		qCritical() << "XmlStore::flush(): could not write to configuration file"
					<< configurationFilePath();
		return;
	}

	QString xml = "<?xml version=\"1.0\"?>\n" + doc.toString( 2 );
	QTextStream( &outfile ) << xml;
}




QString XmlStore::configurationFilePath()
{
	QString base;
	switch( scope() )
	{
		case Global: base = LocalSystem::globalConfigPath(); break;
		case Personal: base = LocalSystem::personalConfigPath(); break;
		case System: base = LocalSystem::systemConfigPath(); break;
	}

	LocalSystem::ensurePathExists( base );

	return base + QDir::separator() + configurationNameFromScope() + ".xml";
}





}

