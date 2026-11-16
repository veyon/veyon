// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QPainter>

#include "ComputerControlListModel.h"
#include "ComputerItemDelegate.h"
#include "FeatureManager.h"


ComputerItemDelegate::ComputerItemDelegate(bool fullLengthLabels, QObject* parent) :
	QStyledItemDelegate(parent),
	m_fullLengthLabels(fullLengthLabels)
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



QSize ComputerItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyleOptionViewItem opt = option;
	const auto style = QApplication::style();

	if (m_fullLengthLabels && index.model())
	{
		QSize maxSize;
		for (int i = 0; i < index.model()->rowCount(); ++i)
		{
			initStyleOption(&opt, index.model()->index(i, 0));
			const auto rowSize = style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), nullptr);
			maxSize = maxSize.expandedTo(rowSize);
		}
		return maxSize;
	}

	initStyleOption(&opt, index);
	const auto size = style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), nullptr);

	return {std::min(size.width(), option.decorationSize.width()), size.height()};
}



void ComputerItemDelegate::initFeaturePixmaps()
{
	for (const auto& feature : VeyonCore::featureManager().features())
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
