/*
 * SystemTrayIcon.h - declaration of SystemTrayIcon class
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

#include "FeatureProviderInterface.h"

class QSystemTrayIcon;
class FeatureWorkerManager;

class VEYON_CORE_EXPORT SystemTrayIcon : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument
	{
		ToolTipText,
		MessageTitle,
		MessageText
	};
	Q_ENUM(Argument)

	explicit SystemTrayIcon( QObject* parent = nullptr );
	~SystemTrayIcon() override = default;

	void setToolTip( const QString& toolTipText,
					 FeatureWorkerManager& featureWorkerManager );

	void showMessage( const QString& messageTitle,
					  const QString& messageText,
					  FeatureWorkerManager& featureWorkerManager );

	Plugin::Uid uid() const override
	{
		return QStringLiteral("3cb1adb1-6b4d-4934-a641-db767df83eea");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "SystemTrayIcon" );
	}

	QString description() const override
	{
		return tr( "System tray icon" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces ) override
	{
		Q_UNUSED(featureUid)
		Q_UNUSED(operation)
		Q_UNUSED(arguments)
		Q_UNUSED(computerControlInterfaces)

		return false;
	}

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

private:
	enum Commands
	{
		SetToolTipCommand,
		ShowMessageCommand
	};

	const Feature m_systemTrayIconFeature;
	const FeatureList m_features;

	QSystemTrayIcon* m_systemTrayIcon;

};
