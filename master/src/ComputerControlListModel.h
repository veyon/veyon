/*
 * ComputerControlListModel.h - data model for computer control objects
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

#ifndef COMPUTER_CONTROL_LIST_MODEL_H
#define COMPUTER_CONTROL_LIST_MODEL_H

#include <QAbstractListModel>
#include <QImage>

#include "ComputerControlInterface.h"

class VeyonMaster;

class ComputerControlListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum {
		UidRole = Qt::UserRole
	};

	ComputerControlListModel( VeyonMaster* masterCore, QObject* parent = nullptr );

	int rowCount( const QModelIndex& parent = QModelIndex() ) const override;

	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;

	void updateComputerScreenSize();

	const ComputerControlInterfaceList& computerControlInterfaces() const
	{
		return m_computerControlInterfaces;
	}

	ComputerControlInterface::Pointer computerControlInterface( const QModelIndex& index ) const;

	Qt::ItemFlags flags( const QModelIndex& index ) const override;

	Qt::DropActions supportedDragActions() const override;
	Qt::DropActions supportedDropActions() const override;


public slots:
	void reload();

signals:
	void rowAboutToBeRemoved( QModelIndex );
	void activeFeaturesChanged( QModelIndex );

private slots:
	void update();

	void updateComputerScreens();

private:
	void startComputerControlInterface( ComputerControlInterface::Pointer controlInterface, QModelIndex index );

	QSize computerScreenSize() const;

	void loadIcons();
	QImage prepareIcon( const QImage& icon );
	QImage computerDecorationRole( ComputerControlInterface::Pointer controlInterface ) const;
	QString computerToolTipRole( ComputerControlInterface::Pointer controlInterface ) const;
	static QString computerDisplayRole( ComputerControlInterface::Pointer controlInterface );
	static QString computerStateDescription( ComputerControlInterface::Pointer controlInterface );
	static QString loggedOnUserInformation( ComputerControlInterface::Pointer controlInterface );
	QString activeFeatures(  ComputerControlInterface::Pointer controlInterface ) const;

	VeyonMaster* m_master;

	QImage m_iconDefault;
	QImage m_iconConnectionProblem;
	QImage m_iconDemoMode;

	ComputerControlInterfaceList m_computerControlInterfaces;

};

#endif // COMPUTER_LIST_MODEL_H
