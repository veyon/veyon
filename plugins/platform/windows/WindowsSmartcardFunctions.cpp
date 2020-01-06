/*
 * WindowsSmartcardFunctions.cpp - header for Smartcard Functions on Windows
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

#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#include <wincred.h>
#include <winscard.h>
#include <winbase.h>
#include <lm.h>

#include "authSSP.h"

#include <cstring>

#include "WindowsSmartcardFunctions.h"
#include "WindowsCoreFunctions.h"
#include "VeyonCore.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

static char szOID_SmartCardLogin[] = "1.3.6.1.4.1.311.20.2.2";
static WCHAR szCryptoProvider[] = L"Microsoft Base Smart Card Crypto Provider";

QString WindowsSmartcardFunctions::upnToUsername( const QString& upn ) const{
    return upnToUsernameStatic(upn);
}

QString WindowsSmartcardFunctions::upnToUsernameStatic( const QString& upn ){
    QString username = upn;

    if (upn.contains(QString::fromLatin1("@"))){
        LPWSTR lpwNT4Name = nullptr;
        DWORD  cchNT4Name = 0;
        if (WindowsSmartcardFunctions::UpnToNT4W(WindowsCoreFunctions::toConstWCharArray( upn ), NULL, &cchNT4Name)) {
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

bool WindowsSmartcardFunctions::UpnToNT4W(LPCWSTR lpwUpnName, LPWSTR lpwNT4Name, LPDWORD cchNT4Name) {
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

QMap<QString,QString> WindowsSmartcardFunctions::smartCardCertificateIds() const{
    vDebug() << "_entry";
    QMap<QString,QString> reply;

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
		vDebug() << "CryptAcquireContextW successful";
		cbData = sizeof hCertStore;
		if (CryptGetProvParam(
				hCryptProv,
				PP_USER_CERTSTORE,
				(BYTE *)&hCertStore,
				&cbData,
				0
				)) 
		{
			vDebug() << "CryptGetProvParam successful";
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

												reply.insert(QString::fromLatin1(baCISN.toHex()),certName);
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
		} else {
			vDebug() << "CryptGetProvParam failed: " << GetLastError();
		}
		CryptReleaseContext(hCryptProv, 0);
	} else {
		vDebug() << "CryptAcquireContextW failed: " << GetLastError();
	}
	return reply;
}

bool WindowsSmartcardFunctions::smartCardAuthenticate( const QString& serializedEntry, const QString& pin ) const{
    vDebug() << "_entry";
    int cNumOIDs = 0;
	LPSTR* rghOIDs = NULL;
	
	CERT_ISSUER_SERIAL_NUMBER cisn;
	QByteArray baCISN = QByteArray::fromHex(serializedEntry.toLatin1());
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
		vDebug() << "CryptAcquireContextW successful";
		cbData = sizeof hCertStore;
		if (CryptGetProvParam(
				hCryptProv,
				PP_USER_CERTSTORE,
				(BYTE *)&hCertStore,
				&cbData,
				0
				)) 
		{
			vDebug() << "CryptGetProvParam successful";
			if ((pCertContext = CertFindCertificateInStore(
				hCertStore,
				PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
				0,
				CERT_FIND_CERT_ID,
				&cid,
				pCertContext)))
			{
				vDebug() << "CertFindCertificateInStore successful";
				if (CertGetValidUsages(
					1,
					&pCertContext,
					&cNumOIDs,
					NULL,
					&cbData
					)) 
				{
					vDebug() << "first CertGetValidUsages successful";
					if (cbData > 0) {
						if ((rghOIDs = (LPSTR*)malloc(cbData)))
						{
							vDebug() << "first CertGetValidUsages malloc successful";
							if (CertGetValidUsages(
								1,
								&pCertContext,
								&cNumOIDs,
								rghOIDs,
								&cbData
								)) 
							{
								vDebug() << "second CertGetValidUsages successful";
								for (int i = 0; i < cNumOIDs; i++) {
									if (strcmp(rghOIDs[i], szOID_SmartCardLogin) == 0) 
									{
										vDebug() << "Found SmartcardLogin Usage Flag";
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
											0, //hCryptProv Set to NULL as per Windows API
											CALG_SHA1, //Algid
											0, //dwFlags
											pCertContext->pbCertEncoded , //pbEncoded
											pCertContext->cbCertEncoded, //cbEncoded
											credInfo.rgbHashOfCert,
											&cbComputedHash
											))									
										{
											vDebug() << "CryptHashCertificate successful";
											LPWSTR MarshaledCredential = NULL;
											if (CredMarshalCredentialW(
												CertCredential, //CredType
												&credInfo, //Credential
												&MarshaledCredential
												))
											{
												vDebug() << "CredMarshalCredentialW successful";
												#if defined(UNICODE) || defined(_UNICODE)
													auto pinEncoded = WindowsCoreFunctions::toWCharArray( pin ).get();
												#else
													auto pinEncoded = pin.toLocal8bit().data();
												#endif
												const auto result = SSPLogonUser(NULL, MarshaledCredential, pinEncoded );
												//delete[] pinEncoded;

												
												if (rghOIDs != NULL) {
													free(rghOIDs);
													rghOIDs = NULL;
												}
												CertFreeCertificateContext(pCertContext);
												CertCloseStore(hCertStore, 0);
												CryptReleaseContext(hCryptProv, 0);

												return result;
											}
										} else {
											vDebug() << "CryptHashCertificate failed: " << GetLastError();
										}
									}
								}
							} else {
								vDebug() << "second CertGetValidUsages failed: " << GetLastError();
							}
							free(rghOIDs);
							rghOIDs = NULL;
						} else {
							vDebug() << "first CertGetValidUsages malloc failed: " << GetLastError();
						}
					} else {
						vDebug() << "first CertGetValidUsages returned size 0";
					}
				} else {
					vDebug() << "first CertGetValidUsages failed: " << GetLastError();
				}
				CertFreeCertificateContext(pCertContext);
			} else {
				vDebug() << "CertFindCertificateInStore failed: " << GetLastError();
			}
			CertCloseStore(hCertStore, 0);
		} else {
			vDebug() << "CryptGetProvParam failed: " << GetLastError();
		}
		CryptReleaseContext(hCryptProv, 0);
	} else {
		vDebug() << "CryptAcquireContextW failed: " << GetLastError();
	}
	vDebug() << "return false";
	return false;
	
}

bool WindowsSmartcardFunctions::smartCardVerifyPin( const QString& serializedEntry, const QString& pin ) const{
	vDebug() << "_entry";
    int cNumOIDs = 0;
	LPSTR* rghOIDs = NULL;
	
	CERT_ISSUER_SERIAL_NUMBER cisn;
	QByteArray baCISN = QByteArray::fromHex(serializedEntry.toLatin1());
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
		vDebug() << "CryptAcquireContextW successful";
		cbData = sizeof hCertStore;
		if (CryptGetProvParam(
				hCryptProv,
				PP_USER_CERTSTORE,
				(BYTE *)&hCertStore,
				&cbData,
				0
				)) 
		{
			vDebug() << "CryptGetProvParam successful";
			if ((pCertContext = CertFindCertificateInStore(
				hCertStore,
				PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
				0,
				CERT_FIND_CERT_ID,
				&cid,
				pCertContext)))
			{
				vDebug() << "CertFindCertificateInStore successful";
				if (CertGetValidUsages(
					1,
					&pCertContext,
					&cNumOIDs,
					NULL,
					&cbData
					)) 
				{
					vDebug() << "first CertGetValidUsages successful";
					if (cbData > 0) {
						if ((rghOIDs = (LPSTR*)malloc(cbData)))
						{
							vDebug() << "first CertGetValidUsages malloc successful";
							if (CertGetValidUsages(
								1,
								&pCertContext,
								&cNumOIDs,
								rghOIDs,
								&cbData
								)) 
							{
								vDebug() << "second CertGetValidUsages successful";
								for (int i = 0; i < cNumOIDs; i++) {
									if (strcmp(rghOIDs[i], szOID_SmartCardLogin) == 0) 
									{
										vDebug() << "Found SmartcardLogin Usage Flag";
										if (CryptSetProvParam(
											hCryptProv,
											PP_SIGNATURE_PIN,
											(BYTE*)pin.toLocal8Bit().data(),
											0
										)) {
												if (rghOIDs != NULL) {
													free(rghOIDs);
													rghOIDs = NULL;
												}
												CertFreeCertificateContext(pCertContext);
												CertCloseStore(hCertStore, 0);
												CryptReleaseContext(hCryptProv, 0);

												return true;
										} else {
											vDebug() << "CryptSetProvParam failed: " << GetLastError();
										}
									}
								}
							} else {
								vDebug() << "second CertGetValidUsages failed: " << GetLastError();
							}
							free(rghOIDs);
							rghOIDs = NULL;
						} else {
							vDebug() << "first CertGetValidUsages malloc failed: " << GetLastError();
						}
					} else {
						vDebug() << "first CertGetValidUsages returned size 0";
					}
				} else {
					vDebug() << "first CertGetValidUsages failed: " << GetLastError();
				}
				CertFreeCertificateContext(pCertContext);
			} else {
				vDebug() << "CertFindCertificateInStore failed: " << GetLastError();
			}
			CertCloseStore(hCertStore, 0);
		} else {
			vDebug() << "CryptGetProvParam failed: " << GetLastError();
		}
		CryptReleaseContext(hCryptProv, 0);
	} else {
		vDebug() << "CryptAcquireContextW failed: " << GetLastError();
	}
	vDebug() << "return false";
	return false;
}

QString WindowsSmartcardFunctions::certificatePem( const QString& serializedEntry) const{
    vDebug() << "_entry";

	QString reply;
	
	CERT_ISSUER_SERIAL_NUMBER cisn;
	QByteArray baCISN = QByteArray::fromHex(serializedEntry.toLatin1());
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
		vDebug() << "CryptAcquireContextW successful";
		cbData = sizeof hCertStore;
		if (CryptGetProvParam(
				hCryptProv,
				PP_USER_CERTSTORE,
				(BYTE *)&hCertStore,
				&cbData,
				0
				)) 
		{
			vDebug() << "CryptGetProvParam successful";
			if ((pCertContext = CertFindCertificateInStore(
				hCertStore,
				PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
				0,
				CERT_FIND_CERT_ID,
				&cid,
				pCertContext)))
			{
				vDebug() << "CertFindCertificateInStore successful";

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
										reply = QString::fromLatin1(x509pem,-1);
										
										free(x509pem);
										x509pem = NULL;
									}  else {
										vDebug() << "x509pem malloc failed: " << GetLastError();
									}
								} else {
									vDebug() << "PEM_write_bio_X509 failed: " << GetLastError();
								}
								BIO_free(mem);
							} else {
								vDebug() << "BIO_new failed: " << GetLastError();
							}
						} else {
							vDebug() << "pbCertEncoded malloc failed: " << GetLastError();
						}
						X509_free(x509Cert);
					}
				}
				 else {
					//TODO add handling for PKCS_7_ASN_ENCODING
					vDebug() << "Only X509_ASN_ENCODING supported at this time" ;
				}
				CertFreeCertificateContext(pCertContext);
			} else {
				vDebug() << "CertFindCertificateInStore failed: " << GetLastError();
			}
			CertCloseStore(hCertStore, 0);
		} else {
			vDebug() << "CryptGetProvParam failed: " << GetLastError();
		}
		CryptReleaseContext(hCryptProv, 0);
	} else {
		vDebug() << "CryptAcquireContextW failed: " << GetLastError();
	}
	vDebug() << "return: " << reply;
	return reply;
	
}

QByteArray WindowsSmartcardFunctions::signWithSmartCardKey(QByteArray data, QCA::SignatureAlgorithm alg, const QString& serializedEntry, const QString& pin ) const{
	QByteArray signature;
	ALG_ID					 algorithm = 0;
	

	switch(alg){
		case QCA::SignatureAlgorithm::EMSA3_SHA1:
			algorithm = CALG_SHA1;
			break;
		case QCA::SignatureAlgorithm::EMSA3_MD5:
			algorithm = CALG_MD5;
			break;
		case QCA::SignatureAlgorithm::EMSA3_MD2:
			algorithm = CALG_MD2;
			break;
		case QCA::SignatureAlgorithm::EMSA3_SHA256:
			algorithm = CALG_SHA_256;
			break;
		case QCA::SignatureAlgorithm::EMSA3_SHA384:
			algorithm = CALG_SHA_384;
			break;
		case QCA::SignatureAlgorithm::EMSA3_SHA512:
			algorithm = CALG_SHA_512;
			break;
		default:
			break;
	}
	
	if (algorithm == 0){
		return signature;
	}
	
	CERT_ISSUER_SERIAL_NUMBER cisn;
	QByteArray baCISN = QByteArray::fromHex(serializedEntry.toLatin1());
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