/*
 * Configuration/JsonStore.cpp - implementation of JsonStore
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "Configuration/JsonStore.h"
#include "Configuration/Object.h"
#include "Filesystem.h"
#include "VeyonConfiguration.h"
#include "PlatformFilesystemFunctions.h"


namespace Configuration
{

JsonStore::JsonStore( Scope scope, const QString &file ) :
	Store( Store::JsonFile, scope ),
	m_file( file )
{
}



static void loadJsonTree( Object* obj, const QJsonObject& jsonParent, const QString& parentKey )
{
	for( auto it = jsonParent.begin(); it != jsonParent.end(); ++it )
	{
		if( it.value().isObject() )
		{
			QJsonObject jsonObject = it.value().toObject();

			if( jsonObject.contains( QStringLiteral( "JsonStoreArray" ) ) )
			{
				obj->setValue( it.key(), jsonObject[QStringLiteral("JsonStoreArray")].toArray(), parentKey );
			}
			else if( jsonObject.contains( QStringLiteral( "JsonStoreObject" ) ) )
			{
				obj->setValue( it.key(), jsonObject[QStringLiteral("JsonStoreObject")].toObject(), parentKey );
			}
			else
			{
				const QString subParentKey = parentKey + ( parentKey.isEmpty() ? QString() : QStringLiteral("/") ) + it.key();
				loadJsonTree( obj, it.value().toObject(), subParentKey );
			}
		}
		else
		{
			obj->setValue( it.key(), it.value().toVariant(), parentKey );
		}
	}
}



void JsonStore::load( Object* obj )
{
	QFile jsonFile( configurationFilePath() );
	if( !jsonFile.open( QFile::ReadOnly ) )
	{
		vWarning() << "could not open" << jsonFile.fileName();
		return;
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson( jsonFile.readAll() );

	loadJsonTree( obj, jsonDoc.object(), {} );
}



static QJsonObject saveJsonTree( const Object::DataMap& dataMap )
{
	QJsonObject jsonData;

	for( auto it = dataMap.begin(); it != dataMap.end(); ++it )
	{
		if( it.value().type() == QVariant::Map )
		{
			jsonData[it.key()] = saveJsonTree( it.value().toMap() );
		}
		else if( static_cast<QMetaType::Type>( it.value().type() ) == QMetaType::QJsonArray )
		{
			QJsonObject jsonObj;
			jsonObj[QStringLiteral("JsonStoreArray")] = it.value().toJsonArray();
			jsonData[it.key()] = jsonObj;
		}
		else if( static_cast<QMetaType::Type>( it.value().type() ) == QMetaType::QJsonObject )
		{
			QJsonObject jsonObj;
			jsonObj[QStringLiteral("JsonStoreObject")] = it.value().toJsonObject();
			jsonData[it.key()] = jsonObj;
		}
		else if( QMetaType( it.value().userType() ).flags().testFlag( QMetaType::IsEnumeration ) )
		{
			jsonData[it.key()] = QJsonValue( it.value().toInt() );
		}
		else
		{
			jsonData[it.key()] = QJsonValue::fromVariant( it.value() );
		}
	}

	return jsonData;
}



void JsonStore::flush( const Object* obj )
{
	QFile outfile( configurationFilePath() );
	if( !outfile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		vCritical() << "could not write to configuration file" << configurationFilePath();
		return;
	}

	outfile.write( QJsonDocument( saveJsonTree( obj->data() ) ).toJson() );
}



bool JsonStore::isWritable() const
{
	QFile outfile( configurationFilePath() );
	outfile.open( QFile::WriteOnly | QFile::Append );
	outfile.close();

	return QFileInfo( configurationFilePath() ).isWritable();

}



void JsonStore::clear()
{
	// truncate configuration file
	QFile outfile( configurationFilePath() );
	outfile.open( QIODevice::WriteOnly | QIODevice::Truncate );
}



QString JsonStore::configurationFilePath() const
{
	if( m_file.isEmpty() == false )
	{
		return m_file;
	}

	QString base;
	switch( scope() )
	{
	case User:
		base = VeyonCore::config().userConfigurationDirectory();
		break;
	case System:
		base = VeyonCore::platform().filesystemFunctions().globalAppDataPath();
		break;
	}

	base = VeyonCore::filesystem().expandPath( base );

	VeyonCore::filesystem().ensurePathExists( base );

	auto fileNameBase = name();
	if( fileNameBase.isEmpty() )
	{
		fileNameBase = configurationNameFromScope();
	}

	return QDir::toNativeSeparators( base + QDir::separator() + fileNameBase + QLatin1String(".json") ); // clazy:exclude=qstring-allocations
}

}
