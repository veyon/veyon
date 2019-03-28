/*
 * WindowsUserFunctions.cpp - implementation of WindowsUserFunctions class
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

#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <wincred.h>
#include <winscard.h>
#include <winbase.h>
#include <lm.h>

#include <cstring>

#include "WindowsCoreFunctions.h"
#include "WindowsUserFunctions.h"
#include "WtsSessionManager.h"

#include "authSSP.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

static char szOID_SmartCardLogin[] = "1.3.6.1.4.1.311.20.2.2";
static WCHAR szCryptoProvider[] = L"Microsoft Base Smart Card Crypto Provider";

QString WindowsUserFunctions::upnToUsername( const QString& upn ){
	return upnToUsernameStatic(upn);
}

QString WindowsUserFunctions::upnToUsernameStatic( const QString& upn ){
	QString username = upn;
	
	if (upn.contains("@")){
		LPWSTR lpwNT4Name = nullptr;
		DWORD  cchNT4Name = 0;
		if (UpnToNT4W(WindowsCoreFunctions::toConstWCharArray( upn ), NULL, &cchNT4Name)) {
			if ((lpwNT4Name = (LPWSTR)malloc(cchNT4Name))){
				if (UpnToNT4W(WindowsCoreFunctions::toConstWCharArray( upn ), lpwNT4Name, &cchNT4Name)) {
					username = QString::fromWCharArray(lpwNT4Name,-1);
				}
				free(lpwNT4Name);
			}	
		}
	}
	
	return username;
}

QString WindowsUserFunctions::fullName( const QString& username )
{
	QString fullName;
	QString nt4Name = upnToUsernameStatic(username);
	

	QString realUsername = nt4Name;
	PBYTE domainController = nullptr;

	const auto nameParts = nt4Name.split( '\\' );
	if( nameParts.size() > 1 )
	{
		realUsername = nameParts[1];
		if( NetGetDCName( nullptr, WindowsCoreFunctions::toConstWCharArray( nameParts[0] ), &domainController ) != NERR_Success )
		{
			domainController = nullptr;
		}
	}

	LPUSER_INFO_2 buf = nullptr;
	NET_API_STATUS nStatus = NetUserGetInfo( reinterpret_cast<LPCWSTR>( domainController ),
											 WindowsCoreFunctions::toConstWCharArray( realUsername ),
											 2, (LPBYTE *) &buf );
	if( nStatus == NERR_Success && buf != nullptr )
	{
		fullName = QString::fromWCharArray( buf->usri2_full_name );
	}

	if( buf != nullptr )
	{
		NetApiBufferFree( buf );
	}

	if( domainController != nullptr )
	{
		NetApiBufferFree( domainController );
	}

	return fullName;
}



QStringList WindowsUserFunctions::userGroups( bool queryDomainGroups )
{
	auto groupList = localUserGroups();

	if( queryDomainGroups )
	{
		groupList.append( domainUserGroups() );
	}

	groupList.removeDuplicates();
	groupList.removeAll( QStringLiteral("") );

	return groupList;
}



QStringList WindowsUserFunctions::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	QString nt4Name = upnToUsernameStatic(username);

	auto groupList = localGroupsOfUser( nt4Name );

	if( queryDomainGroups )
	{
		groupList.append( domainGroupsOfUser( nt4Name ) );
	}

	groupList.removeDuplicates();
	groupList.removeAll( QStringLiteral("") );

	return groupList;
}



QString WindowsUserFunctions::currentUser()
{
	auto sessionId = WtsSessionManager::activeConsoleSession();

	auto username = WtsSessionManager::querySessionInformation( sessionId, WtsSessionManager::SessionInfoUserName );
	auto domainName = WtsSessionManager::querySessionInformation( sessionId, WtsSessionManager::SessionInfoDomainName );

	// check whether we just got the name of the local computer
	if( !domainName.isEmpty() )
	{
		wchar_t computerName[MAX_PATH]; // Flawfinder: ignore
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( domainName == QString::fromWCharArray( computerName ) )
		{
			// reset domain name as we do not need to store local computer name
			domainName = QString();
		}
	}

	if( domainName.isEmpty() )
	{
		return username;
	}

	return domainName + '\\' + username;
}



QStringList WindowsUserFunctions::loggedOnUsers()
{
	return WtsSessionManager::loggedOnUsers();
}



void WindowsUserFunctions::logon( const QString& username, const QString& password )
{
	Q_UNUSED(username);
	Q_UNUSED(password);

	// TODO
}



void WindowsUserFunctions::logout()
{
	ExitWindowsEx( EWX_LOGOFF | (EWX_FORCE | EWX_FORCEIFHUNG), SHTDN_REASON_MAJOR_OTHER );
}



bool WindowsUserFunctions::authenticate( const QString& username, const QString& password )
{
	QString domain;
	QString user;

	const auto userNameParts = username.split( '\\' );
	if( userNameParts.count() == 2 )
	{
		domain = userNameParts[0];
		user = userNameParts[1];
	}
	else
	{
		user = username;
	}

	auto domainWide = WindowsCoreFunctions::toWCharArray( domain );
	auto userWide = WindowsCoreFunctions::toWCharArray( user );
	auto passwordWide = WindowsCoreFunctions::toWCharArray( password );

	const auto result = SSPLogonUser( domainWide, userWide, passwordWide );

	delete[] domainWide;
	delete[] userWide;
	delete[] passwordWide;

	return result;
}


QByteArray WindowsUserFunctions::signWithSmartCardKey(QByteArray data, CryptoCore::SignatureAlgorithm alg, const QVariant& smartCardKeyIdentifier, const QString& pin ){
	QByteArray signature;
	ALG_ID					 algorithm = 0;
	

	switch(alg){
		case CryptoCore::SignatureAlgorithm::EMSA3_SHA1:
			algorithm = CALG_SHA1;
			break;
		case CryptoCore::SignatureAlgorithm::EMSA3_MD5:
			algorithm = CALG_MD5;
			break;
		case CryptoCore::SignatureAlgorithm::EMSA3_MD2:
			algorithm = CALG_MD2;
			break;
		case CryptoCore::SignatureAlgorithm::EMSA3_SHA256:
			algorithm = CALG_SHA_256;
			break;
		case CryptoCore::SignatureAlgorithm::EMSA3_SHA384:
			algorithm = CALG_SHA_384;
			break;
		case CryptoCore::SignatureAlgorithm::EMSA3_SHA512:
			algorithm = CALG_SHA_512;
			break;
		default:
			break;
	}
	
	if (algorithm == 0){
		return signature;
	}
	
	CERT_ISSUER_SERIAL_NUMBER cisn;
	QByteArray baCISN = smartCardKeyIdentifier.toByteArray();
	std::memcpy(&cisn,baCISN.data(),sizeof cisn);
	cisn.Issuer.pbData = (BYTE*)(baCISN.data() + sizeof cisn);
	cisn.SerialNumber.pbData = (BYTE*)(baCISN.data() + sizeof cisn + cisn.Issuer.cbData);

	CERT_ID cid;
	cid.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
	cid.IssuerSerialNumber = cisn;

	
	HCRYPTPROV 			 hCryptProv;
	HCERTSTORE       hCertStore;
	PCCERT_CONTEXT   pCertContext = NULL;
	DWORD            cbData;
	if (CryptAcquireContextW(
			&hCryptProv,               // handle to the CSP
			NULL,                  // container name
			szCryptoProvider,                     
			PROV_RSA_FULL,             // provider type
			CRYPT_SILENT))                        // flag values
	{
		cbData = sizeof hCertStore;
		if (CryptGetProvParam(
				hCryptProv,
				PP_USER_CERTSTORE,
				(BYTE *)&hCertStore,
				&cbData,
				0
				)) 
		{
			if ((pCertContext = CertFindCertificateInStore(
				hCertStore,
				PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
				0,
				CERT_FIND_CERT_ID,
				&cid,
				pCertContext)))
			{
					HCRYPTPROV smartCardProv;
					DWORD dwKeySpec;
					BOOL fCallerFreeProvOrNCryptKey;
					if (CryptAcquireCertificatePrivateKey(
						pCertContext,
						CRYPT_ACQUIRE_SILENT_FLAG,
						NULL,
						&smartCardProv,
						&dwKeySpec,
						&fCallerFreeProvOrNCryptKey
						)) 
					{
							QByteArray pinByteArray = pin.toLatin1();
							BYTE* pinBytes = (BYTE*)pinByteArray.data(); 
							
						if (CryptSetProvParam(
							smartCardProv,
							PP_SIGNATURE_PIN,
							pinBytes,
							0
							))
						{															
							HCRYPTKEY  hUserKey = 0;
							if (CryptGetUserKey(
								smartCardProv,
								dwKeySpec,
								&hUserKey
								)) 
							{
								HCRYPTHASH hHash;
								if (CryptCreateHash(
									smartCardProv,
									algorithm,
									0, //HCRYPTKEY 0 -> Not a keyed algorithm 
									0, //dwFlags
									&hHash
									)) 
								{
									if (CryptHashData(
										hHash,
										(BYTE *)data.data(),
										data.size(), //dwDataLen
										0      //dwFlags
										)) 
									{
										cbData = pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData;
										BYTE* buffer = (BYTE*)malloc(cbData);
										if (CryptSignHashW(
											hHash,
											dwKeySpec,
											NULL,
											0,      //dwFlags,
											buffer, //pbSignature
											&cbData
											)) 
										{
											signature.resize(cbData);
											//CryptSignHashW outputs signature in reverse bytes of standard;
											for (DWORD i = 0; i < cbData; i++){
												signature.data()[i] = buffer[cbData - 1 - i];
											}
										}
									}
									CryptDestroyHash(hHash);
								}
								CryptDestroyKey(hUserKey);
							}
						}										
						if (fCallerFreeProvOrNCryptKey) {
							CryptReleaseContext(smartCardProv, 0);
						}
					}
					CertFreeCertificateContext(pCertContext);
			}
			CertCloseStore(hCertStore, 0);
		}
		CryptReleaseContext(hCryptProv, 0);
	}

	return signature;
}


bool WindowsUserFunctions::smartCardAuthenticate( const QVariant& certificateId, const QString& pin )
{
	int cNumOIDs = 0;
	LPSTR* rghOIDs = NULL;
	
	WCHAR pszUpnString[1024];

	CERT_ISSUER_SERIAL_NUMBER cisn;
	QByteArray baCISN = certificateId.toByteArray();
	std::memcpy(&cisn,baCISN.data(),sizeof cisn);
	cisn.Issuer.pbData = (BYTE*)(baCISN.data() + sizeof cisn);
	cisn.SerialNumber.pbData = (BYTE*)(baCISN.data() + sizeof cisn + cisn.Issuer.cbData);

	CERT_ID cid;
	cid.dwIdChoice = CERT_ID_ISSUER_SERIAL_NUMBER;
	cid.IssuerSerialNumber = cisn;

	HCRYPTPROV 			 hCryptProv;
	HCERTSTORE       hCertStore;
	PCCERT_CONTEXT   pCertContext = NULL;
	DWORD            cbData;
	if (CryptAcquireContextW(
			&hCryptProv,               // handle to the CSP
			NULL,                  // container name
			szCryptoProvider,                     
			PROV_RSA_FULL,             // provider type
			CRYPT_SILENT))                        // flag values
	{
		cbData = sizeof hCertStore;
		if (CryptGetProvParam(
				hCryptProv,
				PP_USER_CERTSTORE,
				(BYTE *)&hCertStore,
				&cbData,
				0
				)) 
		{
			if ((pCertContext = CertFindCertificateInStore(
				hCertStore,
				PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
				0,
				CERT_FIND_CERT_ID,
				&cid,
				pCertContext)))
			{
				if (CertGetValidUsages(
					1,
					&pCertContext,
					&cNumOIDs,
					NULL,
					&cbData
					)) 
				{

					if (cbData > 0) {
						if ((rghOIDs = (LPSTR*)malloc(cbData)))
						{
							if (CertGetValidUsages(
								1,
								&pCertContext,
								&cNumOIDs,
								rghOIDs,
								&cbData
								)) 
							{
								for (int i = 0; i < cNumOIDs; i++) {
									if (strcmp(rghOIDs[i], szOID_SmartCardLogin) == 0) 
									{
										DWORD cbComputedHash = 20;
										CERT_CREDENTIAL_INFO credInfo;
										credInfo.cbSize = sizeof credInfo;
	//CryptHashCertificate2 is not supported on current version of mingw-w64
	//Using deprecated CryptHashCertificate instead
	//Leaving equivalent CryptHashCertificate2 as comment for future update
	/*
										if (CryptHashCertificate2(
											BCRYPT_SHA1_ALGORITHM,
											0, //dwFlags
											NULL, //pvReserved
											pCertContext->pbCertEncoded , //pbEncoded
											pCertContext->cbCertEncoded, //cbEncoded
											credInfo.rgbHashOfCert,
											&cbComputedHash
											))
	*/
										if (CryptHashCertificate(
											NULL, //hCryptProv Set to NULL as per Windows API
											CALG_SHA1, //Algid
											0, //dwFlags
											pCertContext->pbCertEncoded , //pbEncoded
											pCertContext->cbCertEncoded, //cbEncoded
											credInfo.rgbHashOfCert,
											&cbComputedHash
											))									
										{
											LPWSTR MarshaledCredential = NULL;
											if (CredMarshalCredentialW(
												CertCredential, //CredType
												&credInfo, //Credential
												&MarshaledCredential
												))
											{
												auto passwordWide = WindowsCoreFunctions::toWCharArray( pin );
												const auto result = SSPLogonUser( NULL, MarshaledCredential, passwordWide );
												delete[] passwordWide;

												//PEM ENCODE CERTIFICATE if Successful
												if (result)
												{
													if (CertGetNameStringW(
													pCertContext,
													CERT_NAME_UPN_TYPE,
													0,
													NULL,
													pszUpnString,
													512))
													{
														setCertificateUpn(QString::fromUtf16( (const ushort *) pszUpnString ));					
													}
												
													if (pCertContext->dwCertEncodingType == X509_ASN_ENCODING) 
													{
														X509* x509Cert = X509_new();
														if (x509Cert != NULL)
														{

															const unsigned char* pbCertEncoded = NULL;
															if ((pbCertEncoded = (const unsigned char*)malloc(pCertContext->cbCertEncoded)))
															{
																std::memcpy((unsigned char*)pbCertEncoded, pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);
																x509Cert = d2i_X509(&x509Cert, &pbCertEncoded, pCertContext->cbCertEncoded);
																
																BIO* mem = BIO_new(BIO_s_mem());
																if (mem != NULL)
																{
																	if (0 < PEM_write_bio_X509(mem, x509Cert)) 
																	{
																		BUF_MEM *bptr;
																		BIO_get_mem_ptr(mem,&bptr);
																		char* x509pem = NULL;
																		if ((x509pem = (char *)malloc(bptr->length + 1))) 
																		{
																			x509pem[bptr->length] = 0;
																			BIO_read(mem, x509pem, bptr->length);
																			setCertificate(QString::fromLatin1(x509pem,-1));
																			
																			setSmartCardKeyIdentifier(certificateId);
																
																			free(x509pem);
																			x509pem = NULL;
																		}
																	}
																	BIO_free(mem);
																}
															}
															X509_free(x509Cert);
														}
													}
	//TODO add handling for PKCS_7_ASN_ENCODING
												}
												if (rghOIDs != NULL) {
													free(rghOIDs);
													rghOIDs = NULL;
												}
												CertFreeCertificateContext(pCertContext);
												CertCloseStore(hCertStore, 0);
												CryptReleaseContext(hCryptProv, 0);

												return result;
											}
										}
									}
								}
							}
							free(rghOIDs);
							rghOIDs = NULL;
						}
					}
				}
				CertFreeCertificateContext(pCertContext);
			}
			CertCloseStore(hCertStore, 0);
		}
		CryptReleaseContext(hCryptProv, 0);
	}
	return false;
}

QMap<QString,QVariant> WindowsUserFunctions::smartCardCertificateIds()
{
	QMap<QString,QVariant> map;
	HCRYPTPROV hCryptProv;
	DWORD            cbData;
	HCERTSTORE       hCertStore = NULL;
	PCCERT_CONTEXT   pCertContext = NULL;
	WCHAR pszNameString[256];
	WCHAR pszUpnString[1024];
	int cNumOIDs = 0;
	LPSTR* rghOIDs = NULL;
	
	if (CryptAcquireContextW(
			&hCryptProv,               // handle to the CSP
			NULL,                  // container name
			szCryptoProvider,                     
			PROV_RSA_FULL,             // provider type
			CRYPT_SILENT | CRYPT_VERIFYCONTEXT))                        // flag values
	{
		cbData = sizeof hCertStore;
		if (CryptGetProvParam(
				hCryptProv,
				PP_USER_CERTSTORE,
				(BYTE *)&hCertStore,
				&cbData,
				0
				)) 
		{
			while ((pCertContext = CertEnumCertificatesInStore(
				hCertStore,
				pCertContext)))
			{
				if (CertVerifyTimeValidity(NULL,pCertContext->pCertInfo) == 0)
				{
					if (CertGetValidUsages(
					1,
					&pCertContext,
					&cNumOIDs,
					NULL,
					&cbData
					)) 
					{
						if (cbData > 0) 
						{
							if ((rghOIDs = (LPSTR*)malloc(cbData)))
							{
								if ((CertGetValidUsages(
									1,
									&pCertContext,
									&cNumOIDs,
									rghOIDs,
									&cbData
								))) 
								{
									for (int i = 0; i < cNumOIDs; i++)
									{
										if (strcmp(rghOIDs[i], szOID_SmartCardLogin) == 0) 
										{
											if (CertGetNameStringW(
												pCertContext,
												CERT_NAME_SIMPLE_DISPLAY_TYPE,
												0,
												NULL,
												pszNameString,
												128))
											{
												QString certName = QString::fromUtf16( (const ushort *) pszNameString );
												
												if (CertGetNameStringW(
													pCertContext,
													CERT_NAME_UPN_TYPE,
													0,
													NULL,
													pszUpnString,
													512))
												{
													certName.append(QString::fromUtf16( (const ushort *) L" (" ));
													certName.append(QString::fromUtf16( (const ushort *) pszUpnString ));
													certName.append(QString::fromUtf16( (const ushort *) L")" ));							
												}
												CERT_ISSUER_SERIAL_NUMBER cisn;
												cbData = sizeof cisn + pCertContext->pCertInfo->Issuer.cbData + pCertContext->pCertInfo->SerialNumber.cbData;
												cisn.Issuer.cbData = pCertContext->pCertInfo->Issuer.cbData;
												cisn.SerialNumber.cbData = pCertContext->pCertInfo->SerialNumber.cbData;
												QByteArray baCISN(cbData,0);
												std::memcpy(baCISN.data(),&cisn,sizeof cisn);
												std::memcpy(baCISN.data() + sizeof cisn,pCertContext->pCertInfo->Issuer.pbData,pCertContext->pCertInfo->Issuer.cbData);
												std::memcpy(baCISN.data() + sizeof cisn + pCertContext->pCertInfo->Issuer.cbData,pCertContext->pCertInfo->SerialNumber.pbData,pCertContext->pCertInfo->SerialNumber.cbData);

												map.insert(certName,QVariant(baCISN));
											}
										}
									}
								}
								free(rghOIDs);
								rghOIDs = NULL;
							}
						}
					}
				}
			}
			CertFreeCertificateContext(pCertContext);
			CertCloseStore(hCertStore, 0);
		}
		CryptReleaseContext(hCryptProv, 0);
	}
	return map;
}

QString WindowsUserFunctions::domainController()
{
	QString dcName;
	LPBYTE outBuffer = nullptr;

	if( NetGetDCName( nullptr, nullptr, &outBuffer ) == NERR_Success )
	{
		dcName = QString::fromUtf16( (const ushort *) outBuffer );

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not query domain controller name!";
	}

	return dcName;
}



QStringList WindowsUserFunctions::domainUserGroups()
{
	const auto dc = domainController();

	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetGroupEnum( WindowsCoreFunctions::toConstWCharArray( dc ), 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr ) == NERR_Success )
	{
		const GROUP_INFO_0* groupInfos = (GROUP_INFO_0 *) outBuffer;

		groupList.reserve( entriesRead );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( (const ushort *) groupInfos[i].grpi0_name );
		}

		if( entriesRead < totalEntries )
		{
			qWarning() << Q_FUNC_INFO << "not all domain groups fetched";
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not fetch domain groups";
	}

	return groupList;
}



QStringList WindowsUserFunctions::domainGroupsOfUser( const QString& username )
{
	const auto dc = domainController();

	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;
	
	QString nt4Name = upnToUsernameStatic(username);

	const auto usernameWithoutDomain = VeyonCore::stripDomain( nt4Name );

	if( NetUserGetGroups( WindowsCoreFunctions::toConstWCharArray( dc ),
						  WindowsCoreFunctions::toConstWCharArray( usernameWithoutDomain ),
						  0, &outBuffer, MAX_PREFERRED_LENGTH,
						  &entriesRead, &totalEntries ) == NERR_Success )
	{
		const GROUP_USERS_INFO_0* groupUsersInfo = (GROUP_USERS_INFO_0 *) outBuffer;

		groupList.reserve( entriesRead );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( (const ushort *) groupUsersInfo[i].grui0_name );
		}

		if( entriesRead < totalEntries )
		{
			qWarning() << Q_FUNC_INFO << "not all domain groups fetched for user" << username;
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not fetch domain groups for user" << username;
	}

	return groupList;
}



QStringList WindowsUserFunctions::localUserGroups()
{
	QStringList groupList;

	LPBYTE outBuffer = NULL;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetLocalGroupEnum( nullptr, 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr ) == NERR_Success )
	{
		LOCALGROUP_INFO_0* groupInfos = (LOCALGROUP_INFO_0 *) outBuffer;

		groupList.reserve( entriesRead );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( (const ushort *) groupInfos[i].lgrpi0_name );
		}

		if( entriesRead < totalEntries )
		{
			qWarning() << Q_FUNC_INFO << "not all local groups fetched";
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not fetch local groups";
	}

	return groupList;
}



QStringList WindowsUserFunctions::localGroupsOfUser( const QString& username )
{
	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;
	
	QString nt4Name = upnToUsernameStatic(username);
	
	if( NetUserGetLocalGroups( nullptr, WindowsCoreFunctions::toConstWCharArray( nt4Name ),
							   0, 0, &outBuffer, MAX_PREFERRED_LENGTH,
							   &entriesRead, &totalEntries ) == NERR_Success )
	{
		const LOCALGROUP_USERS_INFO_0* localGroupUsersInfo = (LOCALGROUP_USERS_INFO_0 *) outBuffer;

		groupList.reserve( entriesRead );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( (const ushort *) localGroupUsersInfo[i].lgrui0_name );
		}

		if( entriesRead < totalEntries )
		{
			qWarning() << Q_FUNC_INFO << "not all local groups fetched for user" << username;
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		qWarning() << Q_FUNC_INFO << "could not fetch local groups for user" << username;
	}

	return groupList;
}

bool WindowsUserFunctions::UpnToNT4W(LPCWSTR lpwUpnName, LPWSTR lpwNT4Name, LPDWORD cchNT4Name) {
	DWORD cbSid = 0;
	DWORD cchReferencedDomainName = 0;
	DWORD cchName = 0;
	SID_NAME_USE eUse = SidTypeUnknown;
	PSID sid;
	LPWSTR        ReferencedDomainName;
	LPWSTR        Name;

	LookupAccountNameW(
		NULL,
		lpwUpnName,
		NULL,
		&cbSid,
		NULL,
		&cchReferencedDomainName,
		&eUse);

	if ((sid = (PSID)malloc(cbSid))) {
		if ((ReferencedDomainName = (LPWSTR)malloc((cchReferencedDomainName + 1) * 2))) {
			if (LookupAccountNameW(
				NULL,
				lpwUpnName,
				sid,
				&cbSid,
				ReferencedDomainName,
				&cchReferencedDomainName,
				&eUse)) {
				LookupAccountSidW(
					NULL,
					sid,
					NULL,
					&cchName,
					ReferencedDomainName,
					&cchReferencedDomainName,
					&eUse
				);
				if ((Name = (LPWSTR)malloc((cchName + 1) * 2))) {
					if (LookupAccountSidW(
						NULL,
						sid,
						Name,
						&cchName,
						ReferencedDomainName,
						&cchReferencedDomainName,
						&eUse
					)) {
						if (lpwNT4Name == NULL) {
							*cchNT4Name = (cchReferencedDomainName + cchName + 2) * 2;
							return true;
						}
						else {
							if ((2 * (cchReferencedDomainName + cchName + 2)) > *cchNT4Name) {
								free(Name);
								free(ReferencedDomainName);
								free(sid);
								SetLastError(ERROR_INSUFFICIENT_BUFFER);
							}
							else {
								for (DWORD i = 0; i < cchReferencedDomainName; i++)
								{
									lpwNT4Name[i] = ReferencedDomainName[i];
								}
								lpwNT4Name[cchReferencedDomainName] = L'\\';
								for (DWORD i = 0; i < cchName; i++)
								{
									lpwNT4Name[cchReferencedDomainName + 1 + i] = Name[i];
								}
								lpwNT4Name[cchReferencedDomainName + cchName + 1] = 0;
								*cchNT4Name = (cchReferencedDomainName + cchName + 2) * 2 ;
								free(Name);
								free(ReferencedDomainName);
								free(sid);
								return true;
							}
						}
					}
					free(Name);
				}
			}
			free(ReferencedDomainName);
		}
		free(sid);
	}
	return false;
}
