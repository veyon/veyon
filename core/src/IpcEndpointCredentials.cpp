// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QRegularExpression>
#include <QScopeGuard>

#include "IpcEndpointCredentials.h"
#include "Random.h"
#include "StandardInputReader.h"
#include "StandardInputReaderPlatform.h"

#include <cstring>

using Veyon::Crypto::SecureBuffer;


std::optional<IpcEndpointCredentials> IpcEndpointCredentials::generate(QLatin1String endpointPrefix, int sessionId)
{
	const auto randomSuffix = Veyon::Crypto::Random::hex(IpcProtocol::EndpointRandomSize);
	auto token = Veyon::Crypto::Random::bytes(IpcProtocol::TokenSize);
	if (sessionId < 0 || randomSuffix.has_value() == false || token.has_value() == false)
	{
		return {};
	}

	const QString endpointName = endpointPrefix + QString::number(sessionId) + QLatin1Char('-') + *randomSuffix;
	if (isValidEndpointName(endpointName) == false)
	{
		return {};
	}

	return IpcEndpointCredentials(endpointName, std::move(*token));
}



std::optional<IpcEndpointCredentials> IpcEndpointCredentials::parse(SecureBuffer& line)
{
	const auto wipeLine = qScopeGuard([&line]() { line.clear(); });

	const auto prefixSize = qsizetype(std::strlen(IpcProtocol::CredentialsLinePrefix));
	const auto tokenHexSize = IpcProtocol::TokenSize * 2;
	const auto* data = line.constData();
	const auto size = line.size();

	// "VEYONIPC1" SP name SP hex64
	if (size < prefixSize + 1 + 1 + 1 + tokenHexSize ||
		size > IpcProtocol::MaximumCredentialsLineSize ||
		std::memcmp(data, IpcProtocol::CredentialsLinePrefix, size_t(prefixSize)) != 0 ||
		data[prefixSize] != ' ' ||
		data[size - tokenHexSize - 1] != ' ')
	{
		return {};
	}

	const auto nameOffset = prefixSize + 1;
	const auto nameSize = size - tokenHexSize - 1 - nameOffset;
	const auto endpointName = QString::fromLatin1(data + nameOffset, nameSize);
	if (isValidEndpointName(endpointName) == false)
	{
		return {};
	}

	auto token = SecureBuffer::fromHex(data + size - tokenHexSize, tokenHexSize);
	if (token.has_value() == false || token->size() != IpcProtocol::TokenSize)
	{
		return {};
	}

	return IpcEndpointCredentials(endpointName, std::move(*token));
}



IpcStandardInputCredentials IpcEndpointCredentials::readFromStandardInput(int timeoutMs)
{
	using Result = IpcStandardInputCredentials::Result;

	const auto standardInput = StandardInputPlatform::standardInputDescriptor();
	if (standardInput < 0 || StandardInputPlatform::isTerminal(standardInput))
	{
		return {Result::Absent};
	}

	SecureBuffer line;
	switch (StandardInputReader::readSecretLine(standardInput, line, int(IpcProtocol::MaximumCredentialsLineSize),
												timeoutMs))
	{
	case StandardInputReader::SecretLineResult::NoInput:
		return {Result::Absent};
	case StandardInputReader::SecretLineResult::Invalid:
		return {Result::Invalid};
	case StandardInputReader::SecretLineResult::Read:
		break;
	}

	auto credentials = parse(line);
	if (credentials.has_value() == false)
	{
		return {Result::Invalid};
	}

	return {Result::Valid, std::move(credentials)};
}



bool IpcEndpointCredentials::isValidEndpointName(const QString& name)
{
	static const QRegularExpression endpointNameRX(
		QRegularExpression::anchoredPattern(QStringLiteral("veyon-(ipc|worker)-[0-9]{1,9}-[0-9a-f]{8}")));

	return name.size() <= IpcProtocol::MaximumEndpointNameSize &&
		endpointNameRX.match(name).hasMatch();
}



IpcEndpointCredentials IpcEndpointCredentials::clone() const
{
	return IpcEndpointCredentials(m_endpointName, m_token.clone());
}



SecureBuffer IpcEndpointCredentials::toLine() const
{
	const auto tokenHex = m_token.toHex();
	const auto name = m_endpointName.toLatin1();
	const auto prefixSize = qsizetype(std::strlen(IpcProtocol::CredentialsLinePrefix));

	if (tokenHex.has_value() == false)
	{
		return {};
	}

	SecureBuffer line(prefixSize + 1 + name.size() + 1 + tokenHex->size() + 1);
	auto* output = line.data();
	std::memcpy(output, IpcProtocol::CredentialsLinePrefix, size_t(prefixSize));
	output += prefixSize;
	*output++ = ' ';
	std::memcpy(output, name.constData(), size_t(name.size()));
	output += name.size();
	*output++ = ' ';
	std::memcpy(output, tokenHex->constData(), size_t(tokenHex->size()));
	output += tokenHex->size();
	*output = '\n';

	return line;
}
