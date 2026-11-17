// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CryptoPrivate.h"

#include <cstring>
#include <openssl/pem.h>

namespace VeyonCrypto {

CertificateRequest::CertificateRequest() = default;
CertificateRequest::~CertificateRequest() = default;
CertificateRequest::CertificateRequest(const CertificateRequest& other) = default;
CertificateRequest::CertificateRequest(CertificateRequest&& other) noexcept = default;
CertificateRequest& CertificateRequest::operator=(const CertificateRequest& other) = default;
CertificateRequest& CertificateRequest::operator=(CertificateRequest&& other) noexcept = default;



CertificateRequest::CertificateRequest(std::shared_ptr<const Private> d) :
	d(std::move(d))
{
}



std::optional<CertificateRequest> CertificateRequest::create(const PrivateKey& key, const QString& commonName)
{
	static constexpr auto Operation = "CertificateRequest::create";

	const auto nativeKey = Internal::Access::nativeHandle(key);
	if (nativeKey == nullptr || commonName.isEmpty() || commonName.size() > MaximumCommonNameSize)
	{
		Internal::setError(Operation, "null key or invalid common name");
		return std::nullopt;
	}

	const auto subject = Internal::createName(commonName);
	Internal::X509RequestPtr request(X509_REQ_new());
	Internal::EvpMdContextPtr context(EVP_MD_CTX_new());
	if (subject == nullptr || request == nullptr || context == nullptr ||
		X509_REQ_set_version(request.get(), 0) != 1 ||
		X509_REQ_set_subject_name(request.get(), subject.get()) != 1 ||
		X509_REQ_set_pubkey(request.get(), nativeKey) != 1 ||
		Internal::initializeSignatureContext(context.get(), nativeKey, key.algorithm(), true) == false ||
		X509_REQ_sign_ctx(request.get(), context.get()) <= 0)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}

	// re-parse the encoding so that a created request is exactly what a receiver will see
	unsigned char* der = nullptr;
	const auto derSize = i2d_X509_REQ(request.get(), &der);
	if (derSize <= 0)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}
	auto result = fromDer(ByteView(reinterpret_cast<const char*>(der), derSize));
	OPENSSL_free(der);

	return result;
}



std::optional<CertificateRequest> CertificateRequest::fromDer(ByteView der)
{
	static constexpr auto Operation = "CertificateRequest::fromDer";

	if (der.isEmpty() || der.size() > MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	auto input = der.bytes();
	Internal::X509RequestPtr request(d2i_X509_REQ(nullptr, &input, long(der.size())));
	if (request == nullptr)
	{
		Internal::setOpenSslError(Operation);
		return std::nullopt;
	}
	if (input != der.bytes() + der.size())
	{
		Internal::setError(Operation, "trailing data");
		return std::nullopt;
	}

	// re-encoding (including the CertificationRequestInfo) equals the input (see CertificateRequest.h)
	unsigned char* canonical = nullptr;
	const auto canonicalSize = i2d_re_X509_REQ_tbs(request.get(), nullptr) > 0 ? i2d_X509_REQ(request.get(), &canonical) : 0;
	const auto isDer = canonicalSize == der.size() && memcmp(canonical, der.bytes(), size_t(canonicalSize)) == 0;
	OPENSSL_free(canonical);
	if (isDer == false)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "not DER-encoded");
		return std::nullopt;
	}

	return Internal::Access::toCertificateRequest(Operation, std::move(request));
}



std::optional<CertificateRequest> CertificateRequest::fromPem(ByteView pem)
{
	static constexpr auto Operation = "CertificateRequest::fromPem";

	if (pem.isEmpty() || pem.size() > 2 * MaximumEncodedSize)
	{
		Internal::setError(Operation, "invalid size");
		return std::nullopt;
	}

	const auto bio = Internal::readOnlyBio(pem);
	QByteArray der;
	if (bio == nullptr ||
		Internal::readSinglePemBlock(bio.get(), "CERTIFICATE REQUEST", MaximumEncodedSize, der) != Internal::PemReadResult::Block)
	{
		Internal::setOpenSslError(Operation);
		Internal::setError(Operation, "invalid, missing or more than one CERTIFICATE REQUEST block");
		return std::nullopt;
	}

	return fromDer(der);
}



bool CertificateRequest::isNull() const
{
	return d == nullptr;
}



QByteArray CertificateRequest::toDer() const
{
	return d ? d->der : QByteArray{};
}



QByteArray CertificateRequest::toPem() const
{
	if (d == nullptr)
	{
		return {};
	}

	Internal::BioPtr bio(BIO_new(BIO_s_mem()));
	if (bio == nullptr || PEM_write_bio_X509_REQ(bio.get(), d->request.get()) != 1)
	{
		Internal::setOpenSslError("CertificateRequest::toPem");
		return {};
	}

	return Internal::bioContents(bio.get());
}



QString CertificateRequest::subject() const
{
	return d ? Internal::nameToString(X509_REQ_get_subject_name(d->request.get())) : QString{};
}



QString CertificateRequest::subjectCommonName() const
{
	return d ? Internal::commonNameOf(X509_REQ_get_subject_name(d->request.get())) : QString{};
}



PublicKey CertificateRequest::publicKey() const
{
	return d ? d->publicKey : PublicKey{};
}



std::optional<CertificateRequest> Internal::Access::toCertificateRequest(const char* operation, X509RequestPtr request)
{
	// key policy first, then proof of possession with an accepted signature algorithm
	// the SubjectPublicKeyInfo exactly as contained in the request (pin definition)
	const auto nativeKey = X509_REQ_get0_pubkey(request.get());
	unsigned char* spki = nullptr;
	const auto spkiSize = i2d_X509_PUBKEY(X509_REQ_get_X509_PUBKEY(request.get()), &spki);
	auto publicKey = spkiSize > 0 ? PublicKey::fromDer(ByteView(reinterpret_cast<const char*>(spki), spkiSize)) : std::nullopt;
	OPENSSL_free(spki);
	if (nativeKey == nullptr || publicKey.has_value() == false)
	{
		setError(operation, "missing or unsupported public key");
		return std::nullopt;
	}

	const X509_ALGOR* signatureAlgorithm = nullptr;
	X509_REQ_get0_signature(request.get(), nullptr, &signatureAlgorithm);
	if (isAcceptedSignatureAlgorithm(signatureAlgorithm, publicKey->algorithm()) == false)
	{
		setError(operation, "unsupported signature algorithm");
		return std::nullopt;
	}

	if (X509_REQ_verify(request.get(), nativeKey) != 1)
	{
		setOpenSslError(operation);
		setError(operation, "invalid self-signature");
		return std::nullopt;
	}

	unsigned char* der = nullptr;
	const auto derSize = i2d_X509_REQ(request.get(), &der);
	if (derSize <= 0)
	{
		setOpenSslError(operation);
		return std::nullopt;
	}

	auto d = std::make_shared<CertificateRequest::Private>();
	d->request = std::move(request);
	d->der = QByteArray(reinterpret_cast<const char*>(der), derSize);
	d->publicKey = std::move(*publicKey);
	OPENSSL_free(der);

	return CertificateRequest(std::move(d));
}

} // namespace VeyonCrypto
