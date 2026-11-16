// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TlsConfiguration.h"
#include "VeyonConfiguration.h"


template<typename T>
static T failClosedTlsEnumValue(const Configuration::TypedProperty<T>& property, T failClosedValue)
{
	const auto value = TlsConfiguration::enumValue<T>(property.variantValue());
	if (value.has_value())
	{
		return *value;
	}

	vCritical() << "invalid value of" << property.absoluteKey() << "- using"
				<< QMetaEnum::fromType<T>().valueToKey(int(failClosedValue));
	return failClosedValue;
}



VeyonCore::TlsServerMode TlsConfiguration::serverMode(const VeyonConfiguration& configuration)
{
	return failClosedTlsEnumValue(configuration.tlsServerModeProperty(), VeyonCore::TlsServerMode::Required);
}



VeyonCore::TlsClientMode TlsConfiguration::clientMode(const VeyonConfiguration& configuration)
{
	return failClosedTlsEnumValue(configuration.tlsClientModeProperty(), VeyonCore::TlsClientMode::Required);
}



VeyonCore::TlsIpTargetPolicy TlsConfiguration::ipTargetPolicy(const VeyonConfiguration& configuration)
{
	return failClosedTlsEnumValue(configuration.tlsIpTargetPolicyProperty(), VeyonCore::TlsIpTargetPolicy::RequireIpSan);
}
