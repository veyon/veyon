// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QRegularExpression>
#include <QScopeGuard>

#include "CaptureHelperCredentials.h"
#include "Random.h"
#include "StandardInputReader.h"

#include <cstring>
#include <unistd.h>

using Veyon::Crypto::SecureBuffer;


std::optional<SecureBuffer> CaptureHelperCredentials::generatePassword()
{
	auto password = Veyon::Crypto::Random::bytes(PasswordSize);
	if (password.has_value())
	{
		auto* bytes = password->bytes();
		for (qsizetype i = 0; i < password->size(); ++i)
		{
			bytes[i] |= 0x80;
		}
	}

	return password;
}



std::optional<QString> CaptureHelperCredentials::generateSocketName(int sessionId)
{
	const auto randomSuffix = Veyon::Crypto::Random::hex(4);
	if (sessionId < 0 || randomSuffix.has_value() == false)
	{
		return {};
	}

	const QString socketName = QStringLiteral("capture-") + QString::number(sessionId) + QLatin1Char('-') +
		*randomSuffix + QStringLiteral(".sock");
	if (isValidSocketName(socketName) == false)
	{
		return {};
	}

	return socketName;
}



bool CaptureHelperCredentials::isValidSocketName(const QString& socketName)
{
	// anchoredPattern(): "$" would also match before a trailing newline
	static const QRegularExpression socketNameRX(
		QRegularExpression::anchoredPattern(QStringLiteral("capture-[0-9]{1,9}-[0-9a-f]{8}\\.sock")));

	return socketName.size() <= MaximumSocketNameSize && socketNameRX.match(socketName).hasMatch();
}



std::optional<CaptureHelperCredentials> CaptureHelperCredentials::create(SecureBuffer&& password,
																		 const QString& socketName)
{
	if (password.size() != PasswordSize ||
		std::memchr(password.constData(), 0, size_t(VncAuthenticationKeySize)) != nullptr ||
		isValidSocketName(socketName) == false)
	{
		return {};
	}

	return CaptureHelperCredentials(std::move(password), socketName);
}



std::optional<CaptureHelperCredentials> CaptureHelperCredentials::parse(SecureBuffer& line)
{
	const auto wipeLine = qScopeGuard([&line]() { line.clear(); });

	const auto prefixSize = qsizetype(sizeof(LinePrefix)) - 1;
	const auto passwordHexSize = PasswordSize * 2;
	const auto socketNameOffset = prefixSize + 1 + passwordHexSize + 1;
	const auto* data = line.constData();

	// prefix SP hex64 SP name
	if (line.size() <= socketNameOffset ||
		line.size() > MaximumLineSize ||
		std::memcmp(data, LinePrefix, size_t(prefixSize)) != 0 ||
		data[prefixSize] != ' ' ||
		data[socketNameOffset - 1] != ' ')
	{
		return {};
	}

	auto password = SecureBuffer::fromHex(data + prefixSize + 1, passwordHexSize);
	if (password.has_value() == false)
	{
		return {};
	}

	// validated strictly by create(): no path separators, no NUL
	const auto socketName = QString::fromLatin1(data + socketNameOffset, int(line.size() - socketNameOffset));

	return create(std::move(*password), socketName);
}



std::optional<CaptureHelperCredentials> CaptureHelperCredentials::readFromStandardInput(int timeoutMs)
{
	if (isatty(STDIN_FILENO))
	{
		return {};
	}

	SecureBuffer line;
	if (StandardInputReader::readSecretLine(STDIN_FILENO, line, int(MaximumLineSize), timeoutMs) !=
		StandardInputReader::SecretLineResult::Read)
	{
		return {};
	}

	return parse(line);
}



SecureBuffer CaptureHelperCredentials::toLine() const
{
	const auto passwordHex = m_password.toHex();
	if (passwordHex.has_value() == false)
	{
		return {};
	}

	const auto prefixSize = qsizetype(sizeof(LinePrefix)) - 1;
	const auto socketName = m_socketName.toLatin1();

	SecureBuffer line(prefixSize + 1 + passwordHex->size() + 1 + socketName.size() + 1);
	auto* output = line.data();
	std::memcpy(output, LinePrefix, size_t(prefixSize));
	output += prefixSize;
	*output++ = ' ';
	std::memcpy(output, passwordHex->constData(), size_t(passwordHex->size()));
	output += passwordHex->size();
	*output++ = ' ';
	std::memcpy(output, socketName.constData(), size_t(socketName.size()));
	output += socketName.size();
	*output = '\n';

	return line;
}
