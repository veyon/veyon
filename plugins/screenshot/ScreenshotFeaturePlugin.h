/*
 * ScreenshotFeaturePlugin.h - declaration of ScreenshotFeature class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef SCREENSHOT_FEATURE_PLUGIN_H
#define SCREENSHOT_FEATURE_PLUGIN_H

#include "Feature.h"
#include "SimpleFeatureProvider.h"

class ScreenshotFeaturePlugin : public QObject, SimpleFeatureProvider, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.Screenshot")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	ScreenshotFeaturePlugin( QObject* parent = nullptr );
	~ScreenshotFeaturePlugin() override {}

	Plugin::Uid uid() const override
	{
		return QStringLiteral("ee322521-f4fb-482d-b082-82a79003afa7");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral("Screenshot");
	}

	QString description() const override
	{
		return tr( "Take screenshots of computers and save them locally." );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	const FeatureList& featureList() const override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;


private:
	const Feature m_screenshotFeature;
	const FeatureList m_features;

};

#endif // SCREENSHOT_FEATURE_PLUGIN_H
