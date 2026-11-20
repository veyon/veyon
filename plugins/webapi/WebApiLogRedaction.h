// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonArray>
#include <QJsonDocument>

#include "WebApiController.h"

// debug output of the WebAPI never contains credentials or the session token (05-tls.md §7a)
namespace WebApiLogRedaction {

static inline QByteArray toJson(const QVariant& data)
{
	return QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact);
}

static inline QByteArray redactedValue()
{
	return QByteArrayLiteral("<redacted>");
}

// values of headers that are credentials
static inline bool isSensitiveHeader(const QByteArray& name)
{
	static const QByteArrayList sensitiveHeaders{
		WebApiController::connectionUidHeaderFieldName().toLower(),
		QByteArrayLiteral("authorization"),
		QByteArrayLiteral("proxy-authorization"),
		QByteArrayLiteral("cookie"),
		QByteArrayLiteral("set-cookie")};
	return sensitiveHeaders.contains(name.toLower());
}

// Qt 6 (QHttpServerRequest::headers() before Qt 6.8, QHttpHeaders::toListOfPairs() since)
static inline QByteArray redactedHeaders(const QList<QPair<QByteArray, QByteArray>>& headers)
{
	QVariantList data;
	data.reserve(headers.size());
	for (const auto& header : headers)
	{
		// wrapped in a QVariant, QList::append(QList) would append the elements
		data.append(QVariant(QVariantList({header.first,
										   isSensitiveHeader(header.first) ? redactedValue() : header.second})));
	}
	return toJson(data);
}

// Qt 5 (QHttpServerRequest::headers())
static inline QByteArray redactedHeaders(const QVariantMap& headers)
{
	auto data = headers;
	for (auto it = data.begin(); it != data.end(); ++it)
	{
		if (isSensitiveHeader(it.key().toUtf8()))
		{
			it.value() = QString::fromLatin1(redactedValue());
		}
	}
	return toJson(data);
}

// compact JSON of a one-element array without the brackets: strings quoted, containers expanded
static inline QString jsonValue(const QVariant& value)
{
	const auto json = QJsonDocument(QJsonArray{QJsonValue::fromVariant(value)}).toJson(QJsonDocument::Compact);
	return QString::fromUtf8(json.mid(1, json.size() - 2));
}

// keys can come from a client, so they are escaped like values (no line breaks or control characters
// in the log) but without the quotes, and long keys are truncated
static inline QString escapedKey(const QString& key)
{
	static constexpr auto MaximumKeyLength = 64;

	const auto quoted = jsonValue(key.left(MaximumKeyLength));
	auto escaped = quoted.mid(1, quoted.size() - 2);
	if (key.size() > MaximumKeyLength)
	{
		escaped += QStringLiteral("...");
	}
	return escaped;
}

// formats data as "{key=value, ...}" with "<redacted>" instead of the value of every key
// in sensitiveKeys (compared case-insensitively); inline variant of Logger::redacted() of 4.12
static inline QString redacted(const QVariantMap& data, const QStringList& sensitiveKeys)
{
	QStringList entries;
	entries.reserve(data.size());

	for (auto it = data.constBegin(), end = data.constEnd(); it != end; ++it)
	{
		if (sensitiveKeys.contains(it.key(), Qt::CaseInsensitive))
		{
			entries.append(escapedKey(it.key()) + QStringLiteral("=<redacted>"));
		}
		else
		{
			entries.append(escapedKey(it.key()) + QLatin1Char('=') + jsonValue(it.value()));
		}
	}

	return QLatin1Char('{') + entries.join(QStringLiteral(", ")) + QLatin1Char('}');
}

// request bodies carry credentials and feature arguments, e.g. the UserLogin password
static inline QString redactedRequestData(const QVariantMap& data)
{
	// WebApiController::Key::Credentials and Key::Arguments
	return redacted(data, {QStringLiteral("credentials"), QStringLiteral("arguments")});
}

// the response to performAuthentication carries the new session token
static inline QString redactedResponseData(const QVariantMap& data)
{
	return redacted(data, {QString::fromUtf8(WebApiController::connectionUidHeaderFieldName().toLower())});
}

}
