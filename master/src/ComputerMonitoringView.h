/*
 * ComputerMonitoringView.h - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef COMPUTER_MONITORING_VIEW_H
#define COMPUTER_MONITORING_VIEW_H

#include "ComputerListModel.h"

#include <QWidget>

class QMenu;

namespace Ui {
class ComputerMonitoringView;
}

class Feature;
class FeatureManager;
class PersonalConfig;

class ComputerMonitoringView : public QWidget
{
	Q_OBJECT
public:
	ComputerMonitoringView( QWidget *parent = 0 );
	~ComputerMonitoringView();

	void setConfiguration( PersonalConfig& config );
	void setComputerManager( ComputerManager& computerManager );
	void setFeatureManager( FeatureManager& featureManager );

	ComputerControlInterfaceList selectedComputerControlInterfaces();

private slots:
	void showContextMenu( const QPoint& pos );
	void setComputerScreenSize( int size );
	void runFeature( const Feature& feature );

private:
	enum {
		DefaultComputerScreenSize = 160
	};

	Ui::ComputerMonitoringView *ui;

	QMenu* m_featureMenu;
	PersonalConfig* m_config;
	ComputerManager* m_computerManager;
	ComputerListModel* m_computerListModel;
	FeatureManager* m_featureManager;

};

#endif // COMPUTER_MONITORING_VIEW_H
