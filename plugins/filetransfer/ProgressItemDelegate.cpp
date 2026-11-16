// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionProgressBar>

#include "ProgressItemDelegate.h"


ProgressItemDelegate::ProgressItemDelegate(QObject* parent) :
	QStyledItemDelegate(parent)
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

	const QWidget* widget = option.widget;
	QStyle* style = widget ? widget->style() : QApplication::style();
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
