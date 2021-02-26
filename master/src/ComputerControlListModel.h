/*
 * ComputerControlListModel.h - data model for computer control objects
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

#include <QAbstractListModel>
#include <QImage>

#include "ComputerListModel.h"
#include "ComputerControlInterface.h"

class VeyonMaster;

class ComputerControlListModel : public ComputerListModel
{
	Q_OBJECT
public:
	explicit ComputerControlListModel( VeyonMaster* masterCore, QObject* parent = nullptr );

	int rowCount( const QModelIndex& parent = QModelIndex() ) const override;

	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;

	void updateComputerScreenSize();

	QSize computerScreenSize() const
	{
		return m_computerScreenSize;
	}

	const ComputerControlInterfaceList& computerControlInterfaces() const
	{
		return m_computerControlInterfaces;
	}

	ComputerControlInterface::Pointer computerControlInterface( const QModelIndex& index ) const;

	void reload();

Q_SIGNALS:
	void activeFeaturesChanged( QModelIndex );
	void computerScreenSizeChanged();

private:
	void update();

	QModelIndex interfaceIndex( ComputerControlInterface* controlInterface ) const;

	void updateState( const QModelIndex& index );
	void updateScreen( const QModelIndex& index );
	void updateActiveFeatures( const QModelIndex& index );
	void updateUser( const QModelIndex& index );

	void startComputerControlInterface( ComputerControlInterface* controlInterface );
	void stopComputerControlInterface( const ComputerControlInterface::Pointer& controlInterface );

	double averageAspectRatio() const;

	QImage scaleAndAlignIcon( const QImage& icon, QSize size ) const;
	QImage computerDecorationRole( const ComputerControlInterface::Pointer& controlInterface ) const;
	QString computerToolTipRole( const ComputerControlInterface::Pointer& controlInterface ) const;
	QString computerDisplayRole( const ComputerControlInterface::Pointer& controlInterface ) const;
	QString computerSortRole( const ComputerControlInterface::Pointer& controlInterface ) const;
	static QString computerStateDescription( const ComputerControlInterface::Pointer& controlInterface );
	static QString loggedOnUserInformation( const ComputerControlInterface::Pointer& controlInterface );
	QString activeFeatures( const ComputerControlInterface::Pointer& controlInterface ) const;

	VeyonMaster* m_master;

	QImage m_iconDefault;
	QImage m_iconConnectionProblem;
	QImage m_iconServerNotRunning;

	QSize m_computerScreenSize{};

	ComputerControlInterfaceList m_computerControlInterfaces{};

};
