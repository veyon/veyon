/*
 * AuthSmartcardPlugin.cpp - dialog for querying smartcard credentials
 *
 * Copyright (c) 2019 State of New Jersey
 * 
 * Maintained by Austin McGuire <austin.mcguire@jjc.nj.gov> 
 * 
 * This file is part of a fork of Veyon - https://veyon.io
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

/*
 This file contains code from https://github.com/KDE/qca/tree/master/examples/cmssigner.
 The below copyright notice is included to comply with the copyright of that code.

 Copyright (C) 2007 Justin Karneges <justin@affinix.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <QApplication>
#include <QMessageBox>

#include "AuthSmartcardPlugin.h"
#include "AuthSmartcardConfiguration.h"
#include "AuthSmartcardDialog.h"
#include "VariantArrayMessage.h"
#include "VeyonConfiguration.h"
#include "PlatformSmartcardFunctions.h"
#include "Filesystem.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
#include <openssl/asn1.h>
#include <openssl/ossl_typ.h>

#include <sstream>

//#include "AuthSmartcardConfigurationDialog.h"


static char szOID_MicrosoftUPN[] = "1.3.6.1.4.1.311.20.2.3";

AuthSmartcardPlugin::AuthSmartcardPlugin( QObject* parent) :
	QObject( parent ),
	_configuration( &VeyonCore::config() ),
	_smartcardDialog(nullptr),
	_serializedEntry(tr("")),
	_pin(tr("")),
	_username(tr("")),
	_certificatePem(tr(""))
{
	vDebug() << "_entry";
	_lastTokenGeneration = QDateTime::fromMSecsSinceEpoch(0);
	vDebug() << "_exit";
}



bool AuthSmartcardPlugin::initializeCredentials()
{
	vDebug() << "_entry ";
	if( qobject_cast<QApplication *>( QCoreApplication::instance() ) )
	{
		vDebug() << "Create AuthSmartcardDialog";
		_smartcardDialog = new AuthSmartcardDialog( QApplication::activeWindow(), &_configuration );
		if( _smartcardDialog->exec() == AuthSmartcardDialog::Accepted )
		{
			vDebug() << "AuthSmartcardDialog::Accepted";
 			_serializedEntry = _smartcardDialog->serializedEntry();
			_pin = _smartcardDialog->pin();
			_certificatePem = VeyonCore::platform().smartcardFunctions().certificatePem(_serializedEntry);
			//_username = VeyonCore::platform().smartcardFunctions().certificateUpn(_serializedEntry);
			_username = certificateUpn(_certificatePem);
			if (_configuration.convertUpnToLogin()){
					vDebug() << "Convert UPN to Login";
					_username = VeyonCore::platform().smartcardFunctions().upnToUsername(_username);
			}
			vDebug() << "return true";
            return true;
		}
		delete _smartcardDialog;
		_smartcardDialog = nullptr;
		vDebug() << "set smartcardDialog = nullptr";
	}
	vDebug() << "return false";
	return false;
}



bool AuthSmartcardPlugin::hasCredentials() const
{
	vDebug() << "_entry";
	if (!_serializedEntry.isEmpty() && !_pin.isEmpty() && !_username.isEmpty()) {
		vDebug() << "return true";
		return true;
	} 
	vDebug() << "return false";
	return false; 
}



bool AuthSmartcardPlugin::checkCredentials() const
{
	vDebug() << "_entry";
	if( hasCredentials() == false )
	{
		vWarning() << "Invalid password!";
		QMessageBox::critical( QApplication::activeWindow(),
							   authenticationTestTitle(),
							   tr( "Logon failed with given Certificate and Pin. Please correct or "
								   "switch to a different authentication method using the Veyon Configurator." ) );
		vDebug() << "return false";
		return false;
	}
	vDebug() << "return true";
	return true;
}



void AuthSmartcardPlugin::configureCredentials()
{
	vDebug() << "_entry";
	QMessageBox::critical( QApplication::activeWindow(),
		tr( "Configuration error" ),
		tr( "Not Implemented Yet" ) );
	//AuthSmartcardConfigurationDialog().exec();
	vDebug() << "_exit";
}


VncServerClient::AuthState AuthSmartcardPlugin::performAuthentication( VncServerClient* client, VariantArrayMessage& message ) const
{
	vDebug() << "_entry";
	switch( client->authState() )
	{
	case VncServerClient::AuthState::Init:
		vDebug() << "Init";

		if ( !QCA::isSupported( "cert" ) ) {
			vDebug() << "no PKI certificate support" ;
			return VncServerClient::AuthState::Failed;
		}

		client->setPrivateKey( CryptoCore::KeyGenerator().createRSA( CryptoCore::RsaKeySize ) );

		if( VariantArrayMessage( message.ioDevice() ).write( client->privateKey().toPublicKey().toPEM() ).send() )
		{
			vDebug() << "return Move to Stage 1";
			return VncServerClient::AuthState::Stage1;
		}

		vDebug() << "failed to send public key";
		return VncServerClient::AuthState::Failed;

	case VncServerClient::AuthState::Stage1:
	{
		vDebug() << "Stage1";
		auto privateKey = client->privateKey();

		QDateTime lastTokenGeneration = message.read().toDateTime();
		QString authCertificatePem =  message.read().toString();

		QCA::SecureArray encryptedToken(message.read().toByteArray()); // Flawfinder: ignore
		QCA::SecureArray decryptedToken;

		if( privateKey.decrypt( encryptedToken,
								&decryptedToken,
								CryptoCore::DefaultEncryptionAlgorithm ) == false )
		{
			vWarning() << "failed to decrypt Token";
			return VncServerClient::AuthState::Failed;
		}

		if (lastTokenGeneration < QDateTime::currentDateTimeUtc().addSecs(-360)
		|| lastTokenGeneration > QDateTime::currentDateTimeUtc().addSecs(360)
		) {
			vInfo() << "Token is expired";
			return VncServerClient::AuthState::Failed;
		}

		QCA::ConvertResult importResult;
		QCA::Certificate authCertificate = QCA::Certificate::fromPEM(authCertificatePem,&importResult);
		if (importResult != QCA::ConvertGood){
			vInfo() << "PEM import Failed";
			return VncServerClient::AuthState::Failed;
		}

		if (!validateCertificate(authCertificate)){
			vInfo() << "Certificate is not trusted";
			return VncServerClient::AuthState::Failed;
		}

		QCA::PublicKey certificatePublicKey = authCertificate.subjectPublicKey();
		if( certificatePublicKey.canVerify() == false )
		{
			vInfo() << "can't verify with Certificate public key!";
			return VncServerClient::AuthState::Failed;
		}

		QString username = certificateUpn(authCertificatePem);
		if (_configuration.convertUpnToLogin()){
			vDebug() << "Convert UPN to Login";
			username = VeyonCore::platform().smartcardFunctions().upnToUsername(username);
		}

		if (username.isNull() || username.isEmpty()){
			vInfo() << "Certificate does not contain username";
			return VncServerClient::AuthState::Failed;
		}

		vInfo() << "authenticating user: " << username;

		QByteArray buffer;
		buffer.setNum(qToBigEndian<qint64>(lastTokenGeneration.toSecsSinceEpoch()));
		buffer.append(authCertificatePem.toLatin1());

		QByteArray signature = decryptedToken.toByteArray();

		if( certificatePublicKey.verifyMessage( buffer, signature, QCA::SignatureAlgorithm::EMSA3_SHA256) == false )
		{
			vInfo() << "Token failed validation" ;
			return VncServerClient::AuthState::Failed;
		}

		client->setUsername( username ); // Flawfinder: ignore

		vDebug() << "SUCCESS";
		return VncServerClient::AuthState::Successful;
	}

	default:
		vDebug() << "Unknown Auth Stage";
		break;
	}

	vDebug() << "Global FAIL";
	return VncServerClient::AuthState::Failed;

}



bool AuthSmartcardPlugin::authenticate( QIODevice* socket ) const
{
	vDebug() << "_entry";
	VariantArrayMessage publicKeyMessage( socket );
	publicKeyMessage.receive();

	if (_lastTokenGeneration < QDateTime::currentDateTimeUtc().addSecs(-60)){
		_mutex.lock();
		vDebug() << "regenerating token";
		if (_lastTokenGeneration < QDateTime::currentDateTimeUtc().addSecs(-60)){
			QDateTime newTokenGeneration = QDateTime::currentDateTimeUtc();
			QByteArray buffer;
			buffer.setNum(qToBigEndian<qint64>(newTokenGeneration.toSecsSinceEpoch()));
			buffer.append(_certificatePem.toLatin1());
			QByteArray signature = VeyonCore::platform().smartcardFunctions().signWithSmartCardKey(buffer, QCA::SignatureAlgorithm::EMSA3_SHA256, _serializedEntry, _pin );
			_token = QCA::SecureArray(signature);
			_lastTokenGeneration = newTokenGeneration;
		}
		_mutex.unlock();
	}

	if (_token.isNull() || _token.size() == 0){
		vCritical() << " Invalid Token";
		return false;
	}

	auto publicKey = CryptoCore::PublicKey::fromPEM( publicKeyMessage.read().toString() );

	if( publicKey.canEncrypt() == false )
	{
		vCritical() << QThread::currentThreadId() << "can't encrypt with given public key!";
		return false;
	}

	auto encryptedToken = publicKey.encrypt( _token, CryptoCore::DefaultEncryptionAlgorithm );
	if( encryptedToken.isEmpty() )
	{
		vCritical() << QThread::currentThreadId() << "Token encryption failed!";
		return false;
	}

	VariantArrayMessage response( socket );
	response.write( _lastTokenGeneration );
	response.write( _certificatePem );
	response.write( encryptedToken.toByteArray() );
	response.send();

	vDebug() << "return true";
	return true;
}

bool AuthSmartcardPlugin::validateCertificate( QString certificatePem ) const {
		vDebug() << "_entry certificatePem";
		if ( !QCA::isSupported( "cert" ) ) {
			vDebug() << "no PKI certificate support" ;
			return false;
		}
		QCA::ConvertResult importResult;
		QCA::Certificate certificate = QCA::Certificate::fromPEM(certificatePem,&importResult);
		if (importResult != QCA::ConvertGood){
			vDebug() << "PEM import Failed";
			return false;
		}
		QCA::CertificateInfo subject = certificate.subjectInfo();
		for (int i = 0 ; i < subject.values().size(); i++){
			vDebug() << subject.values().at(i);
		}

		return validateCertificate(certificate);
}

bool AuthSmartcardPlugin::validateCertificate( QCA::Certificate certificate ) const {
	vDebug() << "_entry certificate";
	if ( !QCA::isSupported( "cert" ) ) {
			vDebug() << "no PKI certificate support" ;
			return false;
	}
	if ( !QCA::haveSystemStore() ) {
			vDebug() << "System certificates not available" ;
			return false;
	}

	QCA::CertificateCollection intermediateCAs = QCA::CertificateCollection();
	QCA::ConvertResult importResult;
	QString certificatePathSetting = _configuration.intermediateCABaseDir();
	QString tempCertificatePath = VeyonCore::filesystem().expandPath(certificatePathSetting ) + QDir::separator();
	QString certificatePath = QDir::toNativeSeparators(tempCertificatePath);
	vDebug() << certificatePath;
	const auto certificateFiles = QDir( certificatePath ).entryInfoList( QDir::Files | QDir::Readable, QDir::Name );
	for(const auto& certificateFile : certificateFiles)
	{
		if (certificateFile.suffix() == tr("pem")){
			vDebug() << certificateFile.absoluteFilePath();
			QCA::Certificate icaCertificate = QCA::Certificate::fromPEMFile(certificateFile.absoluteFilePath(), &importResult,QString());
			if (importResult == QCA::ConvertGood){
				intermediateCAs.addCertificate(icaCertificate);
			}
		}
	}
	
	QCA::Validity validity = certificate.validate(QCA::systemStore(),intermediateCAs);
	if (validity != QCA::ValidityGood) {
		vDebug() <<  "Invalid Certificate " << validity;
		return false;
	}
	vDebug() <<  "Return Valid Certificate ";
	return true;
}

QString AuthSmartcardPlugin::certificateUpn(QString certificatePem) const{
	vDebug() <<  "_entry";
	QString reply;
	QByteArray in = certificatePem.toLatin1();
	BIO *bi = BIO_new(BIO_s_mem());
	BIO_write(bi, in.data(), in.size());

	X509 *x509 = PEM_read_bio_X509(bi, NULL, 0, NULL);
	if (x509 != nullptr){
		vDebug() <<  "Pem read successfully";
		QList<QString> list;
		//std::vector<std::string> list;
		GENERAL_NAMES* subjectAltNames = (GENERAL_NAMES*)X509_get_ext_d2i(x509, NID_subject_alt_name, NULL, NULL);
		for (int i = 0; i < sk_GENERAL_NAME_num(subjectAltNames); i++)
		{
			vDebug() <<  "subjectAltName " << i;
			GENERAL_NAME* gen = sk_GENERAL_NAME_value(subjectAltNames, i);
			if (gen->type == GEN_EMAIL)
			{
				vDebug() <<  "Email ";
				ASN1_IA5STRING *asn1_str = gen->d.uniformResourceIdentifier;
				std::string san = std::string( (char*)ASN1_STRING_get0_data(asn1_str), ASN1_STRING_length(asn1_str) );
				list.append( QString::fromStdString(san));
			}
			else if (gen->type == GEN_OTHERNAME)
			{
				vDebug() <<  "Other" ;
				if (gen->d.otherName){
					if (OBJ_obj2nid(gen->d.otherName->type_id) == OBJ_txt2nid(szOID_MicrosoftUPN)){
						vDebug() <<  "universalPrincipalName (Microsoft UPN)" ;
						list.append( QString::fromUtf8((char*)ASN1_STRING_get0_data(gen->d.otherName->value->value.utf8string),ASN1_STRING_length(gen->d.otherName->value->value.utf8string)));
					}
				}
				//std::cerr << "Not implemented: parse sans ("<< __FILE__ << ":" << __LINE__ << ")" << endl;
			}
		}
		GENERAL_NAMES_free(subjectAltNames);
		if (list.count() > 0){
			reply = list.first();
		}
	}	
	vDebug() <<  "return: " << reply;
	return reply;
}

IMPLEMENT_CONFIG_PROXY(AuthSmartcardConfiguration)
