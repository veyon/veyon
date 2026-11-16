// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QHelpEvent>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

#include "FlexibleListView.h"


FlexibleListView::FlexibleListView(QWidget* parent) :
	QListView(parent),
	m_uidRole(Qt::UserRole)
{
	connect(this, &QListView::indexesMoved, this, &FlexibleListView::updatePositions);
}



void FlexibleListView::setUidRole(int role)
{
	m_uidRole = role;
}



void FlexibleListView::setFlexible(bool enabled)
{
	if (enabled)
	{
		setMovement(QListView::Free);
	}
	else
	{
		setMovement(QListView::Static);
	}

	doItemsLayout();
}



bool FlexibleListView::flexible() const
{
	return movement() == QListView::Free;
}



void FlexibleListView::alignToGrid()
{
	auto m = model();

	for (int i = 0, count = m->rowCount(); i < count; ++i)
	{
		const auto index = m->index(i, 0);
		const auto uid = m->data(index, m_uidRole).toUuid();

		if (uid.isNull() == false && m_positions.contains(uid))
		{
			m_positions[uid] = QPointF(qMax<int>(0, qRound(m_positions[uid].x())),
									   qMax<int>(0, qRound(m_positions[uid].y())));
			setPositionForIndex(toItemPosition(std::as_const(m_positions)[uid]), index);
		}
	}
}



QJsonArray FlexibleListView::savePositions()
{
	QJsonArray data;

	for (auto it = m_positions.constBegin(), end = m_positions.constEnd(); it != end; ++it)
	{
		QJsonObject object;
		object[QStringLiteral("uid")] = it.key().toString();
		object[QStringLiteral("x")] = it.value().x();
		object[QStringLiteral("y")] = it.value().y();
		data += object;
	}

	return data;
}



void FlexibleListView::loadPositions(const QJsonArray& data)
{
	m_positions.clear();

	for (const auto& item : data)
	{
		const auto object = item.toObject();
		const QUuid uid(object[QStringLiteral("uid")].toString());
		if (uid.isNull() == false)
		{
			m_positions[uid] = QPointF(object[QStringLiteral("x")].toDouble(), object[QStringLiteral("y")].toDouble());
		}
	}
}



void FlexibleListView::doItemsLayout()
{
	QListView::doItemsLayout();

	if (movement() == QListView::Free)
	{
		restorePositions();
	}
}



bool FlexibleListView::viewportEvent(QEvent* event)
{
	const auto ret = QListView::viewportEvent(event);
	if (event->type() == QEvent::ToolTip)
	{
		m_toolTipPos = ret ? static_cast<QHelpEvent*>(event)->pos() : QPoint{};
	}

	return ret;
}



void FlexibleListView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
	QListView::dataChanged(topLeft, bottomRight, roles);

	if (roles.contains(m_uidRole))
	{
		restorePositions();
	}

	if (m_toolTipPos.isNull() == false && (roles.isEmpty() || roles.contains(Qt::ToolTipRole)))
	{
		if (viewport()->mapToGlobal(m_toolTipPos) != QCursor::pos())
		{
			m_toolTipPos = QPoint();
			return;
		}

		const auto index = indexAt(m_toolTipPos);
		if (index.row() >= topLeft.row() && index.row() <= bottomRight.row())
		{
			QHelpEvent he(QEvent::ToolTip, m_toolTipPos, viewport()->mapToGlobal(m_toolTipPos));
			QListView::viewportEvent(&he);
		}
	}
}



void FlexibleListView::restorePositions()
{
	auto m = model();

	if (m == nullptr)
	{
		return;
	}

	for (int i = 0, count = m->rowCount(); i < count; ++i)
	{
		const auto index = m->index(i, 0);
		const auto uid = m->data(index, m_uidRole).toUuid();

		if (uid.isNull() == false && m_positions.contains(uid))
		{
			setPositionForIndex(toItemPosition(std::as_const(m_positions)[uid]), index);
		}
	}
}



void FlexibleListView::updatePositions()
{
	if (movement() == QListView::Free && model())
	{
		auto m = model();

		for (int i = 0, count = m->rowCount(); i < count; ++i)
		{
			const auto index = m->index(i, 0);
			const auto uid = m->data(index, m_uidRole).toUuid();

			if (uid.isNull() == false)
			{
				m_positions[uid] = toGridPoint(rectForIndex(index).topLeft());
			}
		}
	}
}



QSizeF FlexibleListView::effectiveGridSize() const
{
	auto m = model();

	if (m && m->rowCount() > 0)
	{
		QSizeF maxSize;
		for (int i = 0; i < m->rowCount(); ++i)
		{
			maxSize = maxSize.expandedTo(rectForIndex(m->index(i, 0)).size());
		}
		return maxSize + QSize(spacing(), spacing());
	}
	else if (iconSize().isEmpty() == false)
	{
		return iconSize() + QSize(spacing(), spacing());
	}

	return {spacing() + 1.0, spacing() + 1.0};
}



QPointF FlexibleListView::toGridPoint(QPoint pos) const
{
	const auto gridSize = effectiveGridSize();

	return {(pos.x() - spacing()) / gridSize.width(),
			(pos.y() - spacing()) / gridSize.height()};
}



QPoint FlexibleListView::toItemPosition(QPointF gridPoint) const
{
	const auto gridSize = effectiveGridSize();

	return {spacing() + qMax<int>(0, static_cast<int>(gridPoint.x() * gridSize.width())),
			spacing() + qMax<int>(0, static_cast<int>(gridPoint.y() * gridSize.height()))};
}
