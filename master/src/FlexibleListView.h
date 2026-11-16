// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QListView>

class FlexibleListView : public QListView
{
	Q_OBJECT
public:
	explicit FlexibleListView(QWidget* parent = nullptr);
	~FlexibleListView() override = default;

	void setUidRole(int role);

	void setFlexible(bool enabled);
	bool flexible() const;

	void alignToGrid();

	QJsonArray savePositions();
	void loadPositions(const QJsonArray& data);

public:
	void doItemsLayout() override;

protected:
	bool viewportEvent(QEvent* event) override;
	void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
					 const QVector<int>& roles) override;

private:
	void restorePositions();
	void updatePositions();

	QSizeF effectiveGridSize() const;
	QPointF toGridPoint(QPoint pos) const;
	QPoint toItemPosition(QPointF gridPoint) const;

	int m_uidRole;
	QHash<QUuid, QPointF> m_positions;

	QPoint m_toolTipPos;
};
