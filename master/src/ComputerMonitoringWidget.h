/*
 * ComputerMonitoringWidget.h - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include "ComputerControlInterface.h"

#include <QWidget>

class QListView;
class QMenu;

namespace Ui {
class ComputerMonitoringWidget;
}

class ComputerSortFilterProxyModel;
class VeyonMaster;

class ComputerMonitoringWidget : public QWidget
{
	Q_OBJECT
public:
	enum {
		MinimumComputerScreenSize = 50,
		MaximumComputerScreenSize = 1000,
		DefaultComputerScreenSize = 150
	};

	explicit ComputerMonitoringWidget( QWidget *parent = nullptr );
	~ComputerMonitoringWidget() override;

	void setVeyonMaster( VeyonMaster& masterCore );

	ComputerSortFilterProxyModel& dataModel();

	QListView* listView() const;

	ComputerControlInterfaceList selectedComputerControlInterfaces();

	void setSearchFilter( const QString& searchFilter );
	void setFilterPoweredOnComputers( bool enabled );
	void setComputerScreenSize( int size );
	void autoAdjustComputerScreenSize();
	void setUseCustomComputerPositions( bool enabled );
	void alignComputers();

	void showContextMenu( QPoint pos );

	void setIgnoreWheelEvent( bool enabled )
	{
		m_ignoreWheelEvent = enabled;
	}

private:
	void runDoubleClickFeature( const QModelIndex& index );
	void runFeature( const Feature& feature );

	void showEvent( QShowEvent* event ) override;
	void wheelEvent( QWheelEvent* event ) override;

	bool isFeatureOrSubFeatureActive( const ComputerControlInterfaceList& computerControlInterfaces,
									 Feature::Uid featureUid ) const;

	void populateFeatureMenu( const ComputerControlInterfaceList& computerControlInterfaces );
	void addFeatureToMenu( const Feature& feature, const QString& label );
	void addSubFeaturesToMenu( const Feature& parentFeature, const FeatureList& subFeatures, const QString& label );

	Ui::ComputerMonitoringWidget *ui;

	VeyonMaster* m_master;
	QMenu* m_featureMenu;
	bool m_ignoreWheelEvent{false};

Q_SIGNALS:
	void computerScreenSizeAdjusted( int size );

};
