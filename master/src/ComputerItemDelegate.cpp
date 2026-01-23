/*
 * ComputerItemDelegate.cpp - implementation of ComputerItemDelegate

 * Copyright (c) 2025-2026 Tobias Junghans <tobydox@veyon.io>
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

#include <QPainter>

#include "ComputerControlListModel.h"
#include "ComputerItemDelegate.h"
#include "FeatureManager.h"


ComputerItemDelegate::ComputerItemDelegate(QObject* parent) :
	QStyledItemDelegate(parent)
{
	initFeaturePixmaps();
}



void ComputerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyledItemDelegate::paint(painter, option, index);

	if (index.isValid() && index.model())
	{
		drawFeatureIcons(painter, option.rect.topLeft(),
						 index.model()->data(index, ComputerControlListModel::ControlInterfaceRole).value<ComputerControlInterface::Pointer>());
	}
}



void ComputerItemDelegate::initFeaturePixmaps()
{
	for (const auto& feature : VeyonCore::featureManager().features() )
	{
		if (feature.testFlag(Feature::Flag::Master) && !feature.iconUrl().isEmpty())
		{
			m_featurePixmaps[feature.uid()] = QIcon(feature.iconUrl()).pixmap(QSize(OverlayIconSize, OverlayIconSize));
		}
	}
}



void ComputerItemDelegate::drawFeatureIcons(QPainter* painter, const QPoint& pos, ComputerControlInterface::Pointer controlInterface) const
{
	if (painter &&
		controlInterface &&
		controlInterface->state() == ComputerControlInterface::State::Connected)
	{
		auto count = 0;
		for (const auto& feature : controlInterface->activeFeatures())
		{
			if (m_featurePixmaps.contains(feature))
			{
				count++;
			}
		}

		if (count == 0)
		{
			return;
		}

		int x = pos.x() + OverlayIconsPadding;
		const int y = pos.y() + OverlayIconsPadding;

		painter->setRenderHint(QPainter::Antialiasing);
		painter->setBrush(QColor(255, 255, 255, 192));
		painter->setPen(QColor(25, 140, 179));
		painter->drawRoundedRect(QRect(x, y, count * (OverlayIconSize + OverlayIconSpacing), OverlayIconSize),
								 OverlayIconsRadius, OverlayIconsRadius);

		for (const auto& feature : controlInterface->activeFeatures())
		{
			const auto it = m_featurePixmaps.find(feature);
			if (it != m_featurePixmaps.constEnd())
			{
				painter->drawPixmap(QPoint(x, y), *it);
				x += OverlayIconSize + OverlayIconSpacing;
			}
		}
	}
}
