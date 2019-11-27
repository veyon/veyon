/*
 * ComputerMonitoringItem.h - provides a view with computer monitor thumbnails
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "ComputerMonitoringView.h"
#include "FlexibleListView.h"

#include <QQuickItem>

class FlexibleListView;

class ComputerMonitoringItem : public QQuickItem, public ComputerMonitoringView
{
	Q_OBJECT
	Q_PROPERTY(QObject* model READ model CONSTANT)
	Q_PROPERTY(QColor backgroundColor READ backgroundColor NOTIFY backgroundColorChanged)
	Q_PROPERTY(QColor textColor READ textColor NOTIFY textColorChanged)
	Q_PROPERTY(QSize iconSize READ iconSize NOTIFY iconSizeChanged)
	Q_PROPERTY(QString searchFilter WRITE setSearchFilter)
	Q_PROPERTY(QStringList groupFilter WRITE setGroupFilter)
	Q_PROPERTY(QVariantList selectedObjects WRITE setSelectedObjects)
	Q_PROPERTY(int computerScreenSize WRITE setComputerScreenSize)
public:
	enum class ComputerScreenSize {
		MinimumComputerScreenSize = MinimumComputerScreenSize,
		DefaultComputerScreenSize = DefaultComputerScreenSize,
		MaximumComputerScreenSize = MaximumComputerScreenSize,
	};
	Q_ENUM(ComputerScreenSize)

	explicit ComputerMonitoringItem( QQuickItem* parent = nullptr );
	~ComputerMonitoringItem() override = default;

	ComputerControlInterfaceList selectedComputerControlInterfaces() const override;

	void componentComplete() override;

	void autoAdjustComputerScreenSize();

	void setUseCustomComputerPositions( bool enabled ) override;
	void alignComputers() override;

	Q_INVOKABLE void runFeature( QString featureUid );

private:
	QObject* model() const;
	QColor backgroundColor() const;
	QColor textColor() const;
	const QSize& iconSize() const;

	void setColors( const QColor& backgroundColor, const QColor& textColor ) override;
	QJsonArray saveComputerPositions() override;
	bool useCustomComputerPositions() override;
	void loadComputerPositions( const QJsonArray& positions ) override;
	void setIconSize( const QSize& size ) override;

	QVariantList selectedObjects() const;
	void setSelectedObjects( const QVariantList& objects );

	QColor m_backgroundColor;
	QColor m_textColor;
	QSize m_iconSize;

	QList<NetworkObject::Uid> m_selectedObjects;

signals:
	void backgroundColorChanged();
	void textColorChanged();
	void iconSizeChanged();

};
