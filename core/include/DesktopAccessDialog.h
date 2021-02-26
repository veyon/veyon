/*
 * DesktopAccessDialog.h - declaration of DesktopAccessDialog class
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

#include "FeatureProviderInterface.h"

class FeatureWorkerManager;

class VEYON_CORE_EXPORT DesktopAccessDialog : public QObject, public FeatureProviderInterface, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument
	{
		User,
		Host,
		Choice
	};
	Q_ENUM(Argument)

	enum Choice
	{
		ChoiceNone,
		ChoiceYes,
		ChoiceNo,
		ChoiceAlways,
		ChoiceNever,
		ChoiceCount
	} ;
	Q_ENUM(Choice)

	explicit DesktopAccessDialog( QObject* parent = nullptr );
	~DesktopAccessDialog() override = default;

	bool isBusy( FeatureWorkerManager* featureWorkerManager ) const;

	void exec( FeatureWorkerManager* featureWorkerManager, const QString& user, const QString& host );
	void abort( FeatureWorkerManager* featureWorkerManager );

	Choice choice() const
	{
		return m_choice;
	}

	Plugin::Uid uid() const override
	{
		return QStringLiteral("13fb9ce8-afbc-4397-93e3-e01c4d8288c9");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "DesktopAccessDialog" );
	}

	QString description() const override
	{
		return tr( "Desktop access dialog" );
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
	static Choice requestDesktopAccess( const QString& user, const QString& host );

	enum
	{
		DialogTimeout = 30000,
		SleepTime = 100
	};

	enum Commands
	{
		RequestDesktopAccess,
		ReportDesktopAccessChoice
	};

	const Feature m_desktopAccessDialogFeature;
	const FeatureList m_features;

	Choice m_choice;

	QTimer m_abortTimer;

Q_SIGNALS:
	void finished();

};
