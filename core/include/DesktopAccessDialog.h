/*
 * DesktopAccessDialog.h - declaration of DesktopAccessDialog class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef DESKTOP_ACCESS_DIALOG_H
#define DESKTOP_ACCESS_DIALOG_H

#include <QTimer>

#include "SimpleFeatureProvider.h"

class FeatureWorkerManager;

class VEYON_CORE_EXPORT DesktopAccessDialog : public QObject, public SimpleFeatureProvider, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	typedef enum Choices
	{
		ChoiceNone,
		ChoiceYes,
		ChoiceNo,
		ChoiceAlways,
		ChoiceNever,
		ChoiceCount
	} Choice;

	DesktopAccessDialog( QObject* parent = nullptr );
	~DesktopAccessDialog() override {}

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

	bool handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message ) override;

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

	enum Arguments
	{
		UserArgument,
		HostArgument,
		ChoiceArgument
	};

	const Feature m_desktopAccessDialogFeature;
	const FeatureList m_features;

	Choice m_choice;

	QTimer m_abortTimer;

signals:
	void finished();

};

#endif // DESKTOP_ACCESS_DIALOG_H
