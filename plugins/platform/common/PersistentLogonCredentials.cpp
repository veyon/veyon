/*
 * PersistentLogonCredentials.cpp - implementation of PersistentLogonCredentials class
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

#include <QBuffer>

#include "PersistentLogonCredentials.h"
#include "ServiceDataManager.h"
#include "VariantStream.h"


bool PersistentLogonCredentials::read( QString* username, Password* password )
{
	if( username == nullptr || password == nullptr )
	{
		vCritical() << "Invalid input pointers";
		return false;
	}

	auto logonData = ServiceDataManager::read( ServiceDataManager::serviceDataTokenFromEnvironment() );
	if( logonData.isEmpty() )
	{
		vCritical() << "Empty data";
		return false;
	}

	QBuffer logonDataBuffer( &logonData );
	if( logonDataBuffer.open( QBuffer::ReadOnly ) == false )
	{
		vCritical() << "Failed to open buffer";
		return false;
	}

	VariantStream stream( &logonDataBuffer );
	*username = stream.read().toString();
	*password = VeyonCore::cryptoCore().decryptPassword( stream.read().toString() );

	return username->isEmpty() == false && password->isEmpty() == false;
}



bool PersistentLogonCredentials::write( const QString& username, const Password& password )
{
	QBuffer logonDataBuffer;
	if( logonDataBuffer.open( QBuffer::WriteOnly ) == false )
	{
		vCritical() << "Failed to open buffer";
		return false;
	}

	VariantStream stream( &logonDataBuffer );
	stream.write( username );
	stream.write( VeyonCore::cryptoCore().encryptPassword( password ) );

	if( ServiceDataManager::write( ServiceDataManager::serviceDataTokenFromEnvironment(),
								   logonDataBuffer.data() ) == false )
	{
		vCritical() << "Failed to write persistent service data";
		return false;
	}

	return true;
}



bool PersistentLogonCredentials::clear()
{
	return ServiceDataManager::write( ServiceDataManager::serviceDataTokenFromEnvironment(), {} );
}

