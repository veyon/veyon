/*
 * TextMessageFeature.h - declaration of TextMessageFeature class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef TEXT_MESSAGE_FEATURE_H
#define TEXT_MESSAGE_FEATURE_H

#include "Feature.h"
#include "FeatureInterface.h"

class TextMessageFeature : public QObject, FeatureInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.italc-solutions.iTALC.Features.FeatureInterface")
	Q_INTERFACES(FeatureInterface)
public:
	TextMessageFeature();
	virtual ~TextMessageFeature() {}

	const FeatureList& featureList() const override;

	bool startMasterFeature( const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 QWidget* parent ) override;

	bool stopMasterFeature( const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 QWidget* parent ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
									  QIODevice* ioDevice,
									  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message, QIODevice* ioDevice ) override;

private:
	enum FeatureMessageArguments {
		MessageTextArgument,
		MessageIcon,
		FeatureMessageArgumentCount
	};

	Feature m_textMessageFeature;
	FeatureList m_features;

};

#endif // TEXT_MESSAGE_FEATURE_H
