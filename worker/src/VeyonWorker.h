/*
 * VeyonWorker.h - basic implementation of Veyon Worker
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef VEYON_WORKER_H
#define VEYON_WORKER_H

#include "BuiltinFeatures.h"
#include "FeatureManager.h"
#include "VeyonWorkerInterface.h"

class FeatureWorkerManagerConnection;

class VeyonWorker : public QObject, VeyonWorkerInterface
{
	Q_OBJECT
public:
	VeyonWorker( const QString& featureUid, QObject* parent = nullptr );

private:
	VeyonCore m_core;
	BuiltinFeatures m_builtinFeatures;
	FeatureManager m_featureManager;
	FeatureWorkerManagerConnection* m_workerManagerConnection;

} ;

#endif
