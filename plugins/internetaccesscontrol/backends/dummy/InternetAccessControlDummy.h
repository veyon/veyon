/*
 * InternetAccessControlDummy.h - declaration of InternetAccessControlDummy class
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef INTERNET_ACCESS_CONTROL_DUMMY_H
#define INTERNET_ACCESS_CONTROL_DUMMY_H

#include "InternetAccessControlBackendInterface.h"

class InternetAccessControlDummy : public QObject, InternetAccessControlBackendInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.InternetAccessControl.Dummy")
	Q_INTERFACES(PluginInterface InternetAccessControlBackendInterface)
public:
	InternetAccessControlDummy( QObject* parent = nullptr );
	~InternetAccessControlDummy() override;

	Plugin::Uid uid() const override
	{
		return "b0496510-46e9-43e7-a4fa-be14dc0ea2ac";
	}

	QString version() const override
	{
		return QStringLiteral("1.0");
	}

	QString name() const override
	{
		return QStringLiteral("InternetAccessControlDummy");
	}

	QString description() const override
	{
		return tr( "Dummy backend for internet access control" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Veyon Community");
	}

	QString copyright() const override
	{
		return QStringLiteral("Tobias Junghans");
	}

	QWidget* configurationWidget() override;

	void blockInternetAccess() override;
	void unblockInternetAccess() override;

private:

};

#endif // INTERNET_ACCESS_CONTROL_DUMMY_H
