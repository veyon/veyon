// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QJsonDocument>

#include "SecretExport.h"

#include <cstdlib>

static void discardLogMessage(QtMsgType, const QMessageLogContext&, const QString&)
{
}



// parsers of encrypted secret exports (untrusted input: files imported with veyon-cli config import
// --include-secrets in a privileged process, 03-secretstore-config.md §4.6): the document header
// and the decrypted plaintext; documents with cheap KDF parameters are also decrypted
extern "C" int LLVMFuzzerTestOneInput(const char* data, size_t size)
{
	static const auto previousMessageHandler = qInstallMessageHandler(discardLogMessage);
	Q_UNUSED(previousMessageHandler)

	const auto input = QByteArray::fromRawData(data, qsizetype(size));

	const auto secrets = SecretExport::parsePlaintext(input);
	if (secrets.has_value() && secrets->size() > size_t(SecretExport::MaximumSecretCount))
	{
		abort();
	}

	const auto json = QJsonDocument::fromJson(input);
	if (json.isObject() == false)
	{
		return 0;
	}

	const auto document = SecretExport::parseDocument(json.object());
	if (document.has_value() == false)
	{
		return 0;
	}

	const auto isCheap = document->kdf == SecretExport::Kdf::Scrypt &&
		document->scryptParameters.cost <= 1024 && document->scryptParameters.blockSize <= 8 &&
		document->scryptParameters.parallelism == 1;
	if (isCheap)
	{
		static const QByteArray passphrase("fuzzing passphrase");
		// no valid tag can be produced by mutation: decryption must fail
		if (SecretExport::decrypt(json.object(), passphrase).has_value())
		{
			abort();
		}
	}

	return 0;
}
