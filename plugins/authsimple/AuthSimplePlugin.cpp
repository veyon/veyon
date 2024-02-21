/*
 * AuthSimplePlugin.cpp - implementation of AuthSimplePlugin class
 *
 * Copyright (c) 2019-2024 Tobias Junghans <tobydox@veyon.io>
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

#include <QApplication>
#include <QLineEdit>
#include <QMessageBox>

#include "AuthSimplePlugin.h"
#include "AuthSimpleConfiguration.h"
#include "AuthSimpleDialog.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"
#include "Configuration/UiMapping.h"


AuthSimplePlugin::AuthSimplePlugin( QObject* parent ) :
	QObject( parent )
{
}



QWidget* AuthSimplePlugin::createAuthenticationConfigurationWidget()
{
	auto lineEdit = new QLineEdit;
	lineEdit->setEchoMode( QLineEdit::Password );

	AuthSimpleConfiguration config( &VeyonCore::config() );

	Configuration::UiMapping::initWidgetFromProperty( config.passwordProperty(), lineEdit );
	Configuration::UiMapping::connectWidgetToProperty( config.passwordProperty(), lineEdit );

	return lineEdit;
}



bool AuthSimplePlugin::initializeCredentials()
{
	m_password.clear();

	if( qobject_cast<QApplication *>( QCoreApplication::instance() ) )
	{
		AuthSimpleDialog passwordDialog( QApplication::activeWindow() );
		if( passwordDialog.exec() == AuthSimpleDialog::Accepted )
		{
			m_password = passwordDialog.password();

			return true;
		}
	}

	return false;
}



bool AuthSimplePlugin::hasCredentials() const
{
	return m_password.isEmpty() == false;
}



bool AuthSimplePlugin::checkCredentials() const
{
	if( hasCredentials() == false )
	{
		vWarning() << "Invalid password!";

		QMessageBox::critical( QApplication::activeWindow(),
							   authenticationTestTitle(),
							   tr( "The supplied password is wrong. Please enter the correct password or "
								   "switch to a different authentication method using the Veyon Configurator." ) );

		return false;
	}

	return true;
}



VncServerClient::AuthState AuthSimplePlugin::performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const
{
	switch( client->authState() )
	{
	case VncServerClient::AuthState::Init:
		client->setPrivateKey( VeyonCore::cryptoCore().createPrivateKey() );

		if( VariantArrayMessage( message.ioDevice() ).write( client->privateKey().toPublicKey().toPEM() ).send() )
		{
			return VncServerClient::AuthState::Stage1;
		}

		vDebug() << "failed to send public key";
		return VncServerClient::AuthState::Failed;

	case VncServerClient::AuthState::Stage1:
	{
		auto privateKey = client->privateKey();

		CryptoCore::PlaintextPassword encryptedPassword( message.read().toByteArray() ); // Flawfinder: ignore

		CryptoCore::PlaintextPassword decryptedPassword;

		if( privateKey.decrypt( encryptedPassword,
								&decryptedPassword,
								CryptoCore::DefaultEncryptionAlgorithm ) == false )
		{
			vWarning() << "failed to decrypt password";
			return VncServerClient::AuthState::Failed;
		}

		vInfo() << "authenticating user" << client->username();

		AuthSimpleConfiguration config( &VeyonCore::config() );

		if( config.password().plainText() == decryptedPassword )
		{
			vDebug() << "SUCCESS";
			return VncServerClient::AuthState::Successful;
		}

		vDebug() << "FAIL";
		return VncServerClient::AuthState::Failed;
	}

	default:
		break;
	}

	return VncServerClient::AuthState::Failed;

}



bool AuthSimplePlugin::authenticate( QIODevice* socket ) const
{
	VariantArrayMessage publicKeyMessage( socket );
	publicKeyMessage.receive();

	auto publicKey = CryptoCore::PublicKey::fromPEM( publicKeyMessage.read().toString() );

	if( publicKey.canEncrypt() == false )
	{
		vCritical() << QThread::currentThreadId() << "can't encrypt with given public key!";
		return false;
	}

	auto encryptedPassword = publicKey.encrypt( m_password, CryptoCore::DefaultEncryptionAlgorithm );
	if( encryptedPassword.isEmpty() )
	{
		vCritical() << QThread::currentThreadId() << "password encryption failed!";
		return false;
	}

	VariantArrayMessage response( socket );
	response.write( encryptedPassword.toByteArray() );
	response.send();

	return true;
}


IMPLEMENT_CONFIG_PROXY(AuthSimpleConfiguration)
