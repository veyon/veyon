/*
 * ComputerItemDelegate.h - header file for ComputerItemDelegate
 *
 * Copyright (c) 2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QPixmapCache>
#include <QStyledItemDelegate>

#include "ComputerControlInterface.h"

class ComputerItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	ComputerItemDelegate(QObject* parent = nullptr);

	virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
	void initFeaturePixmaps();
	void drawFeatureIcons(QPainter* painter, const QPoint& pos, ComputerControlInterface::Pointer controlInterface) const;

	static constexpr int OverlayIconSize = 32;
	static constexpr int OverlayIconSpacing = 4;
	static constexpr int OverlayIconsPadding = 8;
	static constexpr int OverlayIconsRadius = 6;

	QMap<QUuid, QPixmap> m_featurePixmaps;

};
