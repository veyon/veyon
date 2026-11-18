/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "ClassroomSidebar.h"
#include <functional>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>
#include "ComputerManager.h"
#include "ComputerControlListModel.h"
#include "DesktopIcons.h"
#include "NetworkObjectModel.h"
#include "VeyonMaster.h"

namespace
{
class RoomDelegate : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override { return QSize(210, 49); }
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		painter->save();
		const auto selected = option.state.testFlag(QStyle::State_Selected);
		painter->fillRect(option.rect, option.palette.color(selected ? QPalette::Base : QPalette::AlternateBase));
		if (selected) painter->fillRect(QRect(option.rect.left(), option.rect.top(), 3, option.rect.height()), option.palette.highlight());
		auto font = option.font;
		font.setBold(true);
		painter->setFont(font);
		painter->setPen(option.palette.color(QPalette::Text));
		painter->drawText(option.rect.adjusted(12, 6, -8, -23), Qt::AlignVCenter,
			QFontMetrics(font).elidedText(index.data().toString(), Qt::ElideRight, option.rect.width() - 20));
		font.setBold(false);
		font.setPixelSize(11);
		painter->setFont(font);
		painter->setPen(option.palette.color(QPalette::PlaceholderText));
		painter->drawText(option.rect.adjusted(12, 25, -8, -5), Qt::AlignVCenter, index.data(Qt::UserRole).toString());
		painter->restore();
	}
};
int hostCount(QAbstractItemModel* model, const QModelIndex& parent)
{
	int count = 0;
	if (model->canFetchMore(parent)) model->fetchMore(parent);
	for (int row = 0; row < model->rowCount(parent); ++row)
	{
		const auto child = model->index(row, 0, parent);
		count += child.data(NetworkObjectModel::TypeRole).value<NetworkObject::Type>() == NetworkObject::Type::Host ? 1 : hostCount(model, child);
	}
	return count;
}
}

ClassroomSidebar::ClassroomSidebar(VeyonMaster& master, QLineEdit* search, QWidget* parent) :
	QWidget(parent), m_master(master), m_rooms(new QListWidget(this)), m_refresh(new QTimer(this))
{
	setObjectName(QStringLiteral("classroomSidebar"));
	setFixedWidth(216);
	setAutoFillBackground(true);
	setBackgroundRole(QPalette::AlternateBase);
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	auto searchBox = new QWidget(this);
	auto searchLayout = new QVBoxLayout(searchBox);
	searchLayout->setContentsMargins(10, 10, 10, 10);
	search->setPlaceholderText(tr("Computer or student"));
	search->setClearButtonEnabled(true);
	search->setFixedHeight(30);
	search->setMinimumWidth(0);
	search->setMaximumWidth(QWIDGETSIZE_MAX);
	search->addAction(DesktopIcons::icon(QStringLiteral("search")), QLineEdit::LeadingPosition);
	searchLayout->addWidget(search);
	layout->addWidget(searchBox);
	auto title = new QLabel(tr("ROOMS"), this);
	title->setObjectName(QStringLiteral("sectionLabel"));
	title->setContentsMargins(12, 0, 8, 4);
	layout->addWidget(title);
	m_rooms->setObjectName(QStringLiteral("classroomRooms"));
	m_rooms->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_rooms->setItemDelegate(new RoomDelegate(m_rooms));
	layout->addWidget(m_rooms, 1);
	m_refresh->setSingleShot(true);
	m_refresh->setInterval(0);
	connect(m_refresh, &QTimer::timeout, this, &ClassroomSidebar::rebuild);
	auto model = master.computerManager().computerTreeModel();
	connect(model, &QAbstractItemModel::modelReset, m_refresh, qOverload<>(&QTimer::start));
	connect(model, &QAbstractItemModel::rowsInserted, m_refresh, qOverload<>(&QTimer::start));
	connect(model, &QAbstractItemModel::rowsRemoved, m_refresh, qOverload<>(&QTimer::start));
	connect(model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
		if (roles.isEmpty() || roles.contains(Qt::CheckStateRole) || roles.contains(Qt::DisplayRole)) m_refresh->start();
	});
	connect(&master.computerControlListModel(), &ComputerControlListModel::stateChanged, m_refresh, qOverload<>(&QTimer::start));
	connect(m_rooms, &QListWidget::currentRowChanged, this, &ClassroomSidebar::activate);
	m_refresh->start();
}

void ClassroomSidebar::rebuild()
{
	const QSignalBlocker blocker(m_rooms);
	auto model = m_master.computerManager().computerTreeModel();
	if (model->canFetchMore({})) model->fetchMore({});
	m_indices.clear();
	m_rooms->clear();
	std::function<void(QModelIndex)> append = [&](QModelIndex parent) {
		for (int row = 0; row < model->rowCount(parent); ++row)
		{
			const auto index = model->index(row, 0, parent);
			if (index.data(NetworkObjectModel::TypeRole).value<NetworkObject::Type>() != NetworkObject::Type::Location) continue;
			m_indices.append(index);
			auto item = new QListWidgetItem(index.data().toString(), m_rooms);
			const auto count = hostCount(model, index);
			int online = 0;
			for (const auto& computer : m_master.allComputerControlInterfaces())
				if (computer->computer().location() == index.data().toString() && computer->state() == ComputerControlInterface::State::Connected) ++online;
			const auto uid = index.data(NetworkObjectModel::UidRole).toUuid();
			item->setData(Qt::UserRole, uid == m_current ? tr("%1 / %2 online").arg(online).arg(count) : tr("%1 computers").arg(count));
			if (uid == m_current) m_rooms->setCurrentItem(item);
			append(index);
		}
	};
	append({});
	if (m_current.isNull() && !m_indices.isEmpty()) activate(0);
}

void ClassroomSidebar::activate(int row)
{
	if (row < 0 || row >= m_indices.size() || !m_indices[row].isValid()) return;
	const auto target = m_indices[row];
	const auto uid = target.data(NetworkObjectModel::UidRole).toUuid();
	if (uid == m_current) return;
	m_current = uid;
	auto model = m_master.computerManager().computerTreeModel();
	for (int i = 0; i < model->rowCount(); ++i)
		model->setData(model->index(i, 0), Qt::Unchecked, Qt::CheckStateRole);
	model->setData(target, Qt::Checked, Qt::CheckStateRole);
	m_rooms->setCurrentRow(row);
	Q_EMIT roomChanged(target.data().toString());
	m_refresh->start();
}
