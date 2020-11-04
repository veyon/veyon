/*
 * ComputerControlListModel.h - data model for computer control objects
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

#include <QAbstractListModel>
#include <QQuickImageProvider>
#include <QImage>

#include "ComputerListModel.h"
#include "ComputerControlInterface.h"

class VeyonMaster;

class ComputerControlListModel : public ComputerListModel, public QQuickImageProvider
{
	Q_OBJECT
public:
	explicit ComputerControlListModel( VeyonMaster* masterCore, QObject* parent = nullptr );

	int rowCount( const QModelIndex& parent = QModelIndex() ) const override;

	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;

	bool setData( const QModelIndex& index, const QVariant& value, int role ) override;

	const QString& imageProviderId() const
	{
		return m_imageProviderId;
	}

	QImage requestImage( const QString& id, QSize *size, const QSize &requestedSize ) override;

	void updateComputerScreenSize();

	const ComputerControlInterfaceList& computerControlInterfaces() const
	{
		return m_computerControlInterfaces;
	}

	ComputerControlInterface::Pointer computerControlInterface( const QModelIndex& index ) const;
	ComputerControlInterface::Pointer computerControlInterface( NetworkObject::Uid uid ) const;

	void reload();

Q_SIGNALS:
	void activeFeaturesChanged( QModelIndex );

private:
	void update();

	QModelIndex interfaceIndex( ComputerControlInterface* controlInterface ) const;

	void updateState( const QModelIndex& index );
	void updateScreen( const QModelIndex& index );
	void updateActiveFeatures( const QModelIndex& index );
	void updateUser( const QModelIndex& index );

	void startComputerControlInterface( ComputerControlInterface* controlInterface );
	void stopComputerControlInterface( const ComputerControlInterface::Pointer& controlInterface );

	QSize computerScreenSize() const;

	void loadIcons();
	QImage prepareIcon( const QImage& icon );
	QImage computerDecorationRole( const ComputerControlInterface::Pointer& controlInterface ) const;
	QString computerToolTipRole( const ComputerControlInterface::Pointer& controlInterface ) const;
	QString computerDisplayRole( const ComputerControlInterface::Pointer& controlInterface ) const;
	QString computerSortRole( const ComputerControlInterface::Pointer& controlInterface ) const;
	static QString computerStateDescription( const ComputerControlInterface::Pointer& controlInterface );
	static QString loggedOnUserInformation( const ComputerControlInterface::Pointer& controlInterface );
	QString activeFeatures( const ComputerControlInterface::Pointer& controlInterface ) const;

	VeyonMaster* m_master;

	QString m_imageProviderId{ QStringLiteral("computers") };
	QImage m_iconDefault{};
	QImage m_iconConnectionProblem{};
	QImage m_iconDemoMode{};

	ComputerControlInterfaceList m_computerControlInterfaces{};

};
