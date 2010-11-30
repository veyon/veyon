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
#include "ItalcConfiguration.h"
#include "LocalSystem.h"
#include "Logger.h"


namespace Configuration
{

XmlStore::XmlStore( Scope scope, const QString &file ) :
	Store( Store::XmlFile, scope ),
	m_file( file )
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
	QFile xmlFile( m_file.isEmpty() ? configurationFilePath() : m_file );
	if( !xmlFile.open( QFile::ReadOnly ) || !doc.setContent( &xmlFile ) )
	{
		qWarning() << "Could not open" << xmlFile.fileName();
		return;
	}

	QDomElement root = doc.documentElement();
	loadXmlTree( _obj, root, QString() );
}




static void saveXmlTree( const Object::DataMap & _dataMap,
				QDomDocument & _doc,
				QDomNode & _parentNode )
{
	for( Object::DataMap::ConstIterator it = _dataMap.begin();
						it != _dataMap.end(); ++it )
	{
		if( it.value().type() == QVariant::Map )
		{
			// create a new element with current key as tagname
			QDomNode node = _doc.createElement( it.key() );

			saveXmlTree( it.value().toMap(), _doc, node );
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
	QDomDocument doc( "ItalcXmlStore" );
	const Object::DataMap & data = _obj->data();

	QDomElement root = doc.createElement( configurationNameFromScope() );
	saveXmlTree( data, doc, root );
	doc.appendChild( root );

	QFile outfile( m_file.isEmpty() ? configurationFilePath() : m_file );
	if( !outfile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		qCritical() << "XmlStore::flush(): could not write to configuration file"
					<< configurationFilePath();
		return;
	}

	QTextStream( &outfile ) << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	outfile.write( doc.toByteArray( 2 ) );
	//qDebug() << doc.toString( 2 );
}




bool XmlStore::isWritable() const
{
	return QFileInfo( m_file.isEmpty() ?
							configurationFilePath() : m_file ).isWritable();

}




QString XmlStore::configurationFilePath() const
{
	QString base;
	switch( scope() )
	{
		case Global:
			base = ItalcConfiguration::defaultConfiguration().globalConfigurationPath();
			break;
		case Personal:
			base = ItalcConfiguration::defaultConfiguration().personalConfigurationPath();
			break;
		case System:
			base = LocalSystem::Path::systemConfigDataPath();
			break;
		default:
			base = QDir::homePath();
			break;
	}

	base = LocalSystem::Path::expand( base );

	LocalSystem::Path::ensurePathExists( base );

	return QDTNS( base + QDir::separator() + configurationNameFromScope() + ".xml" );
}





}

