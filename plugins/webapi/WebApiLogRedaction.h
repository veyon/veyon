// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonDocument>

#include "Logger.h"
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

// request bodies carry credentials and feature arguments, e.g. the UserLogin password
static inline QString redactedRequestData(const QVariantMap& data)
{
	// WebApiController::Key::Credentials and Key::Arguments
	return Logger::redacted(data, {QStringLiteral("credentials"), QStringLiteral("arguments")});
}

// the response to performAuthentication carries the new session token
static inline QString redactedResponseData(const QVariantMap& data)
{
	return Logger::redacted(data, {QString::fromUtf8(WebApiController::connectionUidHeaderFieldName().toLower())});
}

}
