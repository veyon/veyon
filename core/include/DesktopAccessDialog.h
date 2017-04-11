/*
 * DesktopAccessDialog.h - declaration of DesktopAccessDialog class
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of veyon - http://veyon.io
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

#ifndef DESKTOP_ACCESS_DIALOG_H
#define DESKTOP_ACCESS_DIALOG_H

#include "DesktopAccessPermission.h"
#include "FeaturePluginInterface.h"

class VEYON_CORE_EXPORT DesktopAccessDialog : public QObject, public FeaturePluginInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeaturePluginInterface PluginInterface)
public:
	DesktopAccessDialog();
	~DesktopAccessDialog() override {}

	DesktopAccessPermission::Choice exec( FeatureWorkerManager& featureWorkerManager,
										  const QString& user,
										  const QString& host,
										  int choiceFlags );

	Plugin::Uid uid() const override
	{
		return "13fb9ce8-afbc-4397-93e3-e01c4d8288c9";
	}

	QString version() const override
	{
		return "1.0";
	}

	QString name() const override
	{
		return "DesktopAccessDialog";
	}

	QString description() const override
	{
		return tr( "Desktop access dialog" );
	}

	QString vendor() const override
	{
		return "veyon Community";
	}

	QString copyright() const override
	{
		return "Tobias Doerffel";
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool startMasterFeature( const Feature& feature,
							 const ComputerControlInterfaceList& computerControlInterfaces,
							 ComputerControlInterface& localComputerControlInterface,
							 QWidget* parent ) override;

	bool handleMasterFeatureMessage( const FeatureMessage& message,
									 ComputerControlInterface& computerControlInterface ) override;

	bool stopMasterFeature( const Feature& feature,
							const ComputerControlInterfaceList& computerControlInterfaces,
							ComputerControlInterface& localComputerControlInterface,
							QWidget* parent ) override;

	bool handleServiceFeatureMessage( const FeatureMessage& message,
									  FeatureWorkerManager& featureWorkerManager ) override;

	bool handleWorkerFeatureMessage( const FeatureMessage& message ) override;

private:
	static DesktopAccessPermission::Choice getDesktopAccessPermission( const QString& user, const QString& host, int choiceFlags );

	enum
	{
		DialogTimeout = 30000,
		SleepTime = 100
	};

	enum Commands
	{
		GetDesktopAccessPermission,
		ReportDesktopAccessPermission
	};

	enum Arguments
	{
		UserArgument,
		HostArgument,
		ChoiceFlagsArgument,
		ResultArgument
	};

	Feature m_desktopAccessDialogFeature;
	FeatureList m_features;

	DesktopAccessPermission::Choice m_result;

};

#endif // DESKTOP_ACCESS_DIALOG_H
