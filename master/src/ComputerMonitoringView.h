/*
 * ComputerMonitoringView.h - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef COMPUTER_MONITORING_VIEW_H
#define COMPUTER_MONITORING_VIEW_H

#include "ComputerListModel.h"
#include "Feature.h"

#include <QSortFilterProxyModel>
#include <QWidget>

class QMenu;

namespace Ui {
class ComputerMonitoringView;
}

class MasterCore;

class ComputerMonitoringView : public QWidget
{
	Q_OBJECT
public:
	enum {
		MinimumComputerScreenSize = 50,
		MaximumComputerScreenSize = 1000,
		DefaultComputerScreenSize = 150
	};

	ComputerMonitoringView( QWidget *parent = nullptr );
	~ComputerMonitoringView() override;

	void setMasterCore( MasterCore& masterCore );

	ComputerControlInterfaceList selectedComputerControlInterfaces();

public slots:
	void setSearchFilter( const QString& searchFilter );
	void setComputerScreenSize( int size );
	void autoAdjustComputerScreenSize();

private slots:
	void runDoubleClickFeature( const QModelIndex& index );
	void showContextMenu( QPoint pos );
	void runFeature( const Feature& feature );

private:
	void showEvent( QShowEvent* event ) override;

	FeatureUidList activeFeatures( const ComputerControlInterfaceList& computerControlInterfaces );

	void populateFeatureMenu( const FeatureUidList& activeFeatures );

	Ui::ComputerMonitoringView *ui;

	MasterCore* m_masterCore;
	QMenu* m_featureMenu;
	ComputerListModel* m_computerListModel;
	QSortFilterProxyModel m_sortFilterProxyModel;

signals:
	void computerScreenSizeAdjusted( int size );

};

#endif // COMPUTER_MONITORING_VIEW_H
