// SPDX-FileCopyrightText: 2025-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPixmapCache>
#include <QStyledItemDelegate>

#include "ComputerControlInterface.h"

class ComputerItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	ComputerItemDelegate(bool fullLengthLabels, QObject* parent = nullptr);

	virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
	void initFeaturePixmaps();
	void drawFeatureIcons(QPainter* painter, const QPoint& pos, ComputerControlInterface::Pointer controlInterface) const;

	static constexpr int OverlayIconSize = 32;
	static constexpr int OverlayIconSpacing = 4;
	static constexpr int OverlayIconsPadding = 8;
	static constexpr int OverlayIconsRadius = 6;

	QMap<QUuid, QPixmap> m_featurePixmaps;
	bool m_fullLengthLabels;
};
