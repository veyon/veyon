/*
 * StudentFeaturesConfiguration.h
 *
 * This file is part of Veyon - https://veyon.io
 */

#pragma once

#include "Configuration.h"

#define FOREACH_STUDENTFEATURES_CONFIG_PROPERTY(macro) \
	macro(AdminPassword, QString, QStringLiteral("veyon_admin"))

class StudentFeaturesConfiguration : public Configuration
{
	Q_OBJECT
	DEFINE_CONFIGURATION_PROPERTIES(FOREACH_STUDENTFEATURES_CONFIG_PROPERTY)

public:
	StudentFeaturesConfiguration(const QString& prefix, QObject* parent = nullptr) :
		Configuration(prefix, parent)
	{
		DEFINE_CONFIGURATION_PROPERTIES_INITIALIZATION(FOREACH_STUDENTFEATURES_CONFIG_PROPERTY)
	}
};
