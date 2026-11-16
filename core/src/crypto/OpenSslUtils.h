// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CryptoError.h"
#include "PublicKey.h"
#include "SecureBuffer.h"

#include <memory>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

// Internal header of the VeyonCrypto layer: never include it from a public header. It is shared by
// the VeyonCrypto sources in veyon-core and by the separate veyon-crypto-authority library.
// Only the EVP/X509 subset of 06-pki.md §2 may be used; no OpenSSL version checks here
// (version-specific code belongs to OpenSslCompat.{h,cpp}). Objects obtained from OpenSSL getters
// are never modified (OpenSSL 4.x returns them const) and getter results are passed on directly or
// via auto so that the code compiles against the 1.1.1 (non-const) and 3.x/4.x (const) signatures.

namespace VeyonCrypto {

namespace Internal {

template<auto FreeFunction>
struct OpenSslDeleter
{
	template<typename T>
	void operator()(T* object) const
	{
		FreeFunction(object);
	}
};

using BigNumPtr = std::unique_ptr<BIGNUM, OpenSslDeleter<BN_clear_free>>;
using BioPtr = std::unique_ptr<BIO, OpenSslDeleter<BIO_free_all>>;
using EvpCipherContextPtr = std::unique_ptr<EVP_CIPHER_CTX, OpenSslDeleter<EVP_CIPHER_CTX_free>>;
using EvpMdContextPtr = std::unique_ptr<EVP_MD_CTX, OpenSslDeleter<EVP_MD_CTX_free>>;
using EvpPkeyContextPtr = std::unique_ptr<EVP_PKEY_CTX, OpenSslDeleter<EVP_PKEY_CTX_free>>;
using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, OpenSslDeleter<EVP_PKEY_free>>;
using Asn1IntegerPtr = std::unique_ptr<ASN1_INTEGER, OpenSslDeleter<ASN1_INTEGER_free>>;
using Asn1TimePtr = std::unique_ptr<ASN1_TIME, OpenSslDeleter<ASN1_TIME_free>>;
using BasicConstraintsPtr = std::unique_ptr<BASIC_CONSTRAINTS, OpenSslDeleter<BASIC_CONSTRAINTS_free>>;
using ExtendedKeyUsagePtr = std::unique_ptr<EXTENDED_KEY_USAGE, OpenSslDeleter<EXTENDED_KEY_USAGE_free>>;
using GeneralNamesPtr = std::unique_ptr<GENERAL_NAMES, OpenSslDeleter<GENERAL_NAMES_free>>;
using GeneralNamePtr = std::unique_ptr<GENERAL_NAME, OpenSslDeleter<GENERAL_NAME_free>>;
using Pkcs8Ptr = std::unique_ptr<PKCS8_PRIV_KEY_INFO, OpenSslDeleter<PKCS8_PRIV_KEY_INFO_free>>;
using X509ExtensionPtr = std::unique_ptr<X509_EXTENSION, OpenSslDeleter<X509_EXTENSION_free>>;
using X509NamePtr = std::unique_ptr<X509_NAME, OpenSslDeleter<X509_NAME_free>>;
using X509Ptr = std::unique_ptr<X509, OpenSslDeleter<X509_free>>;
using X509PubkeyPtr = std::unique_ptr<X509_PUBKEY, OpenSslDeleter<X509_PUBKEY_free>>;
using X509RequestPtr = std::unique_ptr<X509_REQ, OpenSslDeleter<X509_REQ_free>>;

// error reporting into VeyonCrypto::lastError(); operation/reason must be static strings without
// any data; setOpenSslError() appends the OpenSSL reason codes and empties the thread's OpenSSL
// error queue so that no stale errors leak into later operations (e.g. QSslSocket)
VEYON_CORE_EXPORT void setError(const char* operation, const char* reason);
VEYON_CORE_EXPORT void setOpenSslError(const char* operation);

// PEM password callback that never prompts (OpenSSL's default reads from the terminal)
VEYON_CORE_EXPORT int noPasswordCallback(char* buffer, int size, int writing, void* userData);

[[nodiscard]] VEYON_CORE_EXPORT bool fitsInt(qsizetype size);

// read-only BIO over the given bytes (no copy, data must outlive the BIO)
[[nodiscard]] VEYON_CORE_EXPORT BioPtr readOnlyBio(ByteView data);
[[nodiscard]] VEYON_CORE_EXPORT QByteArray bioContents(BIO* bio);

// Reads the next PEM block with the given label (or its legacy form "X509 CERTIFICATE" or "NEW
// CERTIFICATE REQUEST", RFC 7468; blocks with other labels are skipped) and returns its decoded
// content. Blocks with RFC 1421 headers (e.g. "Proc-Type: 4,ENCRYPTED") or more than
// maximumSize bytes are errors; all PEM parsers use this so that the DER content goes through the
// same strict fromDer() checks and limits as DER input.
enum class PemReadResult {
	Block,
	EndOfInput,
	Error,
};
[[nodiscard]] VEYON_CORE_EXPORT PemReadResult readPemBlock(BIO* bio, const char* label, qsizetype maximumSize, QByteArray& der);
// same, but further blocks with the given label are an error
[[nodiscard]] VEYON_CORE_EXPORT PemReadResult readSinglePemBlock(BIO* bio, const char* label, qsizetype maximumSize, QByteArray& der);

// DER encoding of a SubjectPublicKeyInfo; empty on failure
[[nodiscard]] VEYON_CORE_EXPORT QByteArray subjectPublicKeyInfo(EVP_PKEY* key);

// whether subjectPublicKeyInfo is exactly OpenSSL's encoding of the key decoded from it, i.e. also
// the encoding inside the BIT STRING (e.g. the RSAPublicKey) is canonical DER; EC keys keep the
// point form of their encoding
[[nodiscard]] VEYON_CORE_EXPORT bool isCanonicalSubjectPublicKeyInfo(EVP_PKEY* key, ByteView subjectPublicKeyInfo);

// the key policy of VeyonCrypto: EC P-256 (named curve), Ed25519 or RSA with
// PrivateKey::MinimumRsaBits..MaximumRsaBits; everything else is KeyAlgorithm::Invalid
[[nodiscard]] VEYON_CORE_EXPORT KeyAlgorithm classifyKey(EVP_PKEY* key);

// the only signature schemes: ECDSA-SHA256 (EC P-256), Ed25519, RSA-PSS-SHA256 (MGF1-SHA256,
// salt length 32); the scheme is bound to the key algorithm and never chosen by the caller
[[nodiscard]] VEYON_CORE_EXPORT bool initializeSignatureContext(EVP_MD_CTX* context, EVP_PKEY* key,
																KeyAlgorithm algorithm, bool forSigning);

// display-safe RFC 2253 representation (UTF-8; control, format and line/paragraph separator
// characters escaped as a backslash, "u" and four hex digits, or "U" and eight hex digits outside
// the Basic Multilingual Plane, e.g. tag characters; literal backslashes are escaped as "\\" by
// RFC 2253) and the single common name of an X.509 name (empty if there is none or more than one,
// if it is longer than 64 characters or contains such characters)
[[nodiscard]] VEYON_CORE_EXPORT QString nameToString(const X509_NAME* name);
[[nodiscard]] VEYON_CORE_EXPORT QString commonNameOf(const X509_NAME* name);

// whether a certificate/CSR signature AlgorithmIdentifier is acceptable for an issuer key of the
// given algorithm, each in its one encoding: ecdsa-with-SHA256 (EcP256) and Ed25519 without
// parameters, sha256WithRSAEncryption with NULL parameters or RSASSA-PSS with exactly SHA-256,
// MGF1-SHA256, salt length 32 and the (omitted) default trailer field in DER (Rsa)
[[nodiscard]] VEYON_CORE_EXPORT bool isAcceptedSignatureAlgorithm(const X509_ALGOR* signatureAlgorithm,
																  KeyAlgorithm issuerKeyAlgorithm);

// X.509 name with CN (required, 1..64 characters) and optional O (0..64 characters); nullptr if invalid
[[nodiscard]] VEYON_CORE_EXPORT X509NamePtr createName(const QString& commonName, const QString& organization = {});

} // namespace Internal

} // namespace VeyonCrypto
