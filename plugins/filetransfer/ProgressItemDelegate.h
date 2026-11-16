// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QStyledItemDelegate>

class ProgressItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit ProgressItemDelegate(QObject* parent = nullptr);

	void paint(QPainter* painter,
			   const QStyleOptionViewItem& option,
			   const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& option,
				   const QModelIndex& index) const override;
};
