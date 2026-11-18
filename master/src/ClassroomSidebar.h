/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <QWidget>
#include <QPersistentModelIndex>
#include <QUuid>
class QListWidget;
class QLineEdit;
class QTimer;
class VeyonMaster;
class ClassroomSidebar : public QWidget
{
	Q_OBJECT
public:
	ClassroomSidebar(VeyonMaster& master, QLineEdit* search, QWidget* parent = nullptr);
Q_SIGNALS:
	void roomChanged(const QString& name);
private:
	void rebuild();
	void activate(int row);
	VeyonMaster& m_master;
	QListWidget* m_rooms;
	QTimer* m_refresh;
	QList<QPersistentModelIndex> m_indices;
	QUuid m_current;
};
