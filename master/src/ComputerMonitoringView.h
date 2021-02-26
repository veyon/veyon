/*
 * ComputerMonitoringView.h - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QTimer>

#include "ComputerControlInterface.h"

class ComputerMonitoringModel;
class VeyonMaster;

// clazy:excludeall=copyable-polymorphic

class ComputerMonitoringView
{
public:
	static constexpr int MinimumComputerScreenSize = 50;
	static constexpr int MaximumComputerScreenSize = 1000;
	static constexpr int DefaultComputerScreenSize = 150;

	static constexpr auto IconSizeAdjustStepSize = 10;
	static constexpr auto IconSizeAdjustDelay = 250;

	ComputerMonitoringView();
	virtual ~ComputerMonitoringView() = default;

	void initializeView( QObject* self );

	void saveConfiguration();

	ComputerMonitoringModel* dataModel() const;

	virtual ComputerControlInterfaceList selectedComputerControlInterfaces() const = 0;
	ComputerControlInterfaceList filteredComputerControlInterfaces() const;

	QString searchFilter() const;
	void setSearchFilter( const QString& searchFilter );

	void setFilterPoweredOnComputers( bool enabled );

	void setComputerScreenSize( int size );
	int computerScreenSize() const;

	virtual void alignComputers() = 0;

	bool autoAdjustIconSize() const
	{
		return m_autoAdjustIconSize;
	}
	void setAutoAdjustIconSize( bool enabled );

protected:
	virtual void setColors( const QColor& backgroundColor, const QColor& textColor ) = 0;
	virtual QJsonArray saveComputerPositions() = 0;
	virtual bool useCustomComputerPositions() = 0;
	virtual void loadComputerPositions( const QJsonArray& positions ) = 0;
	virtual void setUseCustomComputerPositions( bool enabled ) = 0;
	virtual void setIconSize( const QSize& size ) = 0;

	virtual bool performIconSizeAutoAdjust();

	void initiateIconSizeAutoAdjust();

	VeyonMaster* master() const
	{
		return m_master;
	}

	void runFeature( const Feature& feature );

	bool isFeatureOrSubFeatureActive( const ComputerControlInterfaceList& computerControlInterfaces,
									 Feature::Uid featureUid ) const;

private:
	VeyonMaster* m_master{nullptr};
	int m_computerScreenSize{DefaultComputerScreenSize};

	bool m_autoAdjustIconSize{false};
	QTimer m_iconSizeAutoAdjustTimer{};

};
