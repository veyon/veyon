// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QMetaEnum>
#include <QVariant>

#include "VeyonCore.h"

#include <cmath>
#include <optional>

class VeyonConfiguration;

// Fail-closed readers for the TLS mode and policy keys (03-secretstore-config.md §5).
//
// The generic accessors (VeyonConfiguration::tlsServerMode(), tlsClientMode(), tlsIpTargetPolicy())
// read every value they cannot convert as the first enumerator, i.e. TLS/ServerMode=Requierd or
// TLS/ClientMode=Bogus would silently become Disabled. They must not be used for TLS decisions
// (session server and demo server listeners: serverMode(), WP6). Security decisions must therefore use these functions instead, which
// accept the enumerator name (QSettings, JSON files, GPO), the number (LocalStore/JsonStore persist
// enums as integers, also as numeric string "1" from INI files or `veyon-cli config set`) and
// replace anything else by the strictest value with an error log line.
class VEYON_CORE_EXPORT TlsConfiguration
{
public:
	[[nodiscard]] static VeyonCore::TlsServerMode serverMode(const VeyonConfiguration& configuration);
	[[nodiscard]] static VeyonCore::TlsClientMode clientMode(const VeyonConfiguration& configuration);
	[[nodiscard]] static VeyonCore::TlsIpTargetPolicy ipTargetPolicy(const VeyonConfiguration& configuration);

	// exact enumerator name, integral number or numeric string of a defined enumerator; std::nullopt
	// for anything else (other names, other case, booleans, fractions, out-of-range numbers)
	template<typename T>
	[[nodiscard]] static std::optional<T> enumValue(const QVariant& value)
	{
		const auto metaEnum = QMetaEnum::fromType<T>();
		const auto type = value.userType();

		if (type == QMetaType::QString || type == QMetaType::QByteArray)
		{
			const auto string = value.toString();
			for (int index = 0; index < metaEnum.keyCount(); ++index)
			{
				if (string == QLatin1String(metaEnum.key(index)))
				{
					return T(metaEnum.value(index));
				}
			}
			bool isNumber = false;
			const auto number = string.toInt(&isNumber);
			return enumFromNumber<T>(isNumber, number);
		}

		if (type == QMetaType::Bool || value.isValid() == false)
		{
			return std::nullopt;
		}

		if (type == QMetaType::Double || type == QMetaType::Float)
		{
			const auto number = value.toDouble();
			const auto isIntegral = std::isfinite(number) && std::floor(number) == number &&
				std::abs(number) <= 1024;
			return enumFromNumber<T>(isIntegral, int(number));
		}

		// integer types and the enum type itself (values set through the typed accessors)
		bool isNumber = false;
		const auto number = value.toInt(&isNumber);
		return enumFromNumber<T>(isNumber, number);
	}

private:
	template<typename T>
	static std::optional<T> enumFromNumber(bool isNumber, int number)
	{
		if (isNumber && QMetaEnum::fromType<T>().valueToKey(number) != nullptr)
		{
			return T(number);
		}
		return std::nullopt;
	}
};
