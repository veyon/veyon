// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QSortFilterProxyModel>
#include <QTimer>

#include "ComputerControlInterface.h"

class SlideshowModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	SlideshowModel(QAbstractItemModel* sourceModel, QObject* parent = nullptr);

	void setIconSize(QSize size);

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

public:
	void setRunning(bool running, int duration);
	void showPrevious();
	void showNext();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
	void setCurrentRow(int row);

	QSize m_iconSize;

	QTimer m_timer;

	int m_currentRow{0};
	ComputerControlInterface::Pointer m_currentControlInterface;
};
