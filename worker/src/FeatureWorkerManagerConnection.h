/*
 * FeatureWorkerManagerConnection.h - class which handles communication between worker manager and worker
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
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

#pragma once

#include <QTcpSocket>

#include "Feature.h"

class FeatureManager;
class FeatureMessage;
class VeyonWorkerInterface;

class FeatureWorkerManagerConnection : public QObject
{
	Q_OBJECT
public:
	FeatureWorkerManagerConnection( VeyonWorkerInterface& worker,
									FeatureManager& featureManager,
									Feature::Uid featureUid,
									QObject* parent = nullptr );


	bool sendMessage( const FeatureMessage& message );

private:
	void sendInitMessage();
	void receiveMessage();

	VeyonWorkerInterface& m_worker;
	FeatureManager& m_featureManager;
	QTcpSocket m_socket;
	Feature::Uid m_featureUid;

} ;
