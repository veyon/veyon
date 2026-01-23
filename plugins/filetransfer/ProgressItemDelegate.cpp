/*
 * ProgressItemDelegate.cpp - implementation of ProgressItemDelegate class
 *
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

#include <QApplication>
#include <QPainter>
#include <QStyleOptionProgressBar>
#include <QStyle>

#include "ProgressItemDelegate.h"


ProgressItemDelegate::ProgressItemDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}



void ProgressItemDelegate::paint(QPainter* painter,
								 const QStyleOptionViewItem& option,
								 const QModelIndex& index) const
{
	QVariant v = index.data(Qt::DisplayRole);
	int progress = v.isValid() ? v.toInt() : 0;
	progress = qBound(0, progress, 100);

	QStyleOptionViewItem opt = option;
	initStyleOption(&opt, index);

	const QWidget *widget = option.widget;
	QStyle *style = widget ? widget->style() : QApplication::style();
	style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

	QStyleOptionProgressBar progressOpt;
	progressOpt.state = opt.state | QStyle::StateFlag::State_Horizontal;
	progressOpt.direction = opt.direction;
	progressOpt.bottomToTop = false;
	progressOpt.rect = opt.rect.adjusted(4, 4, -4, -4);
	progressOpt.fontMetrics = opt.fontMetrics;
	progressOpt.minimum = 0;
	progressOpt.maximum = 100;
	progressOpt.progress = progress;
	progressOpt.text = QString::number(progress) + QStringLiteral("%");
	progressOpt.textVisible = true;
	progressOpt.textAlignment = Qt::AlignCenter;

	style->drawControl(QStyle::CE_ProgressBar, &progressOpt, painter, widget);
}



QSize ProgressItemDelegate::sizeHint(const QStyleOptionViewItem& option,
									 const QModelIndex& index) const
{
	const auto baseSize = QStyledItemDelegate::sizeHint(option, index);

	return QSize(baseSize.width(), std::max(baseSize.height(), 24));
}
