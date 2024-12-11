/*
    SPDX-FileCopyrightText: 2015 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: David Faure <david.faure@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KExtraColumnsProxyModel.h"

#include <QItemSelection>

class KExtraColumnsProxyModelPrivate
{
    Q_DECLARE_PUBLIC(KExtraColumnsProxyModel)
    KExtraColumnsProxyModel *const q_ptr;

public:
    KExtraColumnsProxyModelPrivate(KExtraColumnsProxyModel *model)
        : q_ptr(model)
    {
    }

    void _ec_sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint);
    void _ec_sourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint);

    // Configuration (doesn't change once source model is plugged in)
    QVector<QString> m_extraHeaders;

    // for layoutAboutToBeChanged/layoutChanged
    QVector<QPersistentModelIndex> layoutChangePersistentIndexes;
    QVector<int> layoutChangeProxyColumns;
    QModelIndexList proxyIndexes;
};

KExtraColumnsProxyModel::KExtraColumnsProxyModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , d_ptr(new KExtraColumnsProxyModelPrivate(this))
{
    // The handling of persistent model indexes assumes mapToSource can be called for any index
    // This breaks for the extra column, so we'll have to do it ourselves
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    setHandleSourceLayoutChanges(false);
#endif
}

KExtraColumnsProxyModel::~KExtraColumnsProxyModel()
{
}

void KExtraColumnsProxyModel::appendColumn(const QString &header)
{
    Q_D(KExtraColumnsProxyModel);
    d->m_extraHeaders.append(header);
}

void KExtraColumnsProxyModel::removeExtraColumn(int idx)
{
    Q_D(KExtraColumnsProxyModel);
    d->m_extraHeaders.remove(idx);
}

bool KExtraColumnsProxyModel::setExtraColumnData(const QModelIndex &parent, int row, int extraColumn, const QVariant &data, int role)
{
    Q_UNUSED(parent);
    Q_UNUSED(row);
    Q_UNUSED(extraColumn);
    Q_UNUSED(data);
    Q_UNUSED(role);
    return false;
}

void KExtraColumnsProxyModel::extraColumnDataChanged(const QModelIndex &parent, int row, int extraColumn, const QVector<int> &roles)
{
    const QModelIndex idx = index(row, proxyColumnForExtraColumn(extraColumn), parent);
    Q_EMIT dataChanged(idx, idx, roles);
}

void KExtraColumnsProxyModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel()) {
        disconnect(sourceModel(),
                   SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)),
                   this,
                   SLOT(_ec_sourceLayoutAboutToBeChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));
        disconnect(sourceModel(),
                   SIGNAL(layoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)),
                   this,
                   SLOT(_ec_sourceLayoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));
    }

    QIdentityProxyModel::setSourceModel(model);

    if (model) {
        // The handling of persistent model indexes assumes mapToSource can be called for any index
        // This breaks for the extra column, so we'll have to do it ourselves
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
        disconnect(model,
                   SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)),
                   this,
                   SLOT(_q_sourceLayoutAboutToBeChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));
        disconnect(model,
                   SIGNAL(layoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)),
                   this,
                   SLOT(_q_sourceLayoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));
#endif
        connect(model,
                SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)),
                this,
                SLOT(_ec_sourceLayoutAboutToBeChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));
        connect(model,
                SIGNAL(layoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)),
                this,
                SLOT(_ec_sourceLayoutChanged(QList<QPersistentModelIndex>, QAbstractItemModel::LayoutChangeHint)));
    }
}

QModelIndex KExtraColumnsProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid()) { // happens in e.g. rowCount(mapToSource(parent))
        return QModelIndex();
    }
    const int column = proxyIndex.column();
    if (column >= sourceModel()->columnCount()) {
        return QModelIndex();
    }
    return QIdentityProxyModel::mapToSource(proxyIndex);
}

QModelIndex KExtraColumnsProxyModel::buddy(const QModelIndex &proxyIndex) const
{
    if (sourceModel()) {
        const int column = proxyIndex.column();
        if (column >= sourceModel()->columnCount()) {
            return proxyIndex;
        }
    }
    return QIdentityProxyModel::buddy(proxyIndex);
}

QModelIndex KExtraColumnsProxyModel::sibling(int row, int column, const QModelIndex &idx) const
{
    if (row == idx.row() && column == idx.column()) {
        return idx;
    }
    return index(row, column, parent(idx));
}

QItemSelection KExtraColumnsProxyModel::mapSelectionToSource(const QItemSelection &selection) const
{
    QItemSelection sourceSelection;

    if (!sourceModel()) {
        return sourceSelection;
    }

    // mapToSource will give invalid index for our additional columns, so truncate the selection
    // to the columns known by the source model
    const int sourceColumnCount = sourceModel()->columnCount();
    QItemSelection::const_iterator it = selection.constBegin();
    const QItemSelection::const_iterator end = selection.constEnd();
    for (; it != end; ++it) {
        Q_ASSERT(it->model() == this);
        QModelIndex topLeft = it->topLeft();
        Q_ASSERT(topLeft.isValid());
        Q_ASSERT(topLeft.model() == this);
        topLeft = topLeft.sibling(topLeft.row(), 0);
        QModelIndex bottomRight = it->bottomRight();
        Q_ASSERT(bottomRight.isValid());
        Q_ASSERT(bottomRight.model() == this);
        if (bottomRight.column() >= sourceColumnCount) {
            bottomRight = bottomRight.sibling(bottomRight.row(), sourceColumnCount - 1);
        }
        // This can lead to duplicate source indexes, so use merge().
        const QItemSelectionRange range(mapToSource(topLeft), mapToSource(bottomRight));
        QItemSelection newSelection;
        newSelection << range;
        sourceSelection.merge(newSelection, QItemSelectionModel::Select);
    }

    return sourceSelection;
}

int KExtraColumnsProxyModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const KExtraColumnsProxyModel);
    return QIdentityProxyModel::columnCount(parent) + d->m_extraHeaders.count();
}

QVariant KExtraColumnsProxyModel::data(const QModelIndex &index, int role) const
{
    Q_D(const KExtraColumnsProxyModel);
    const int extraCol = extraColumnForProxyColumn(index.column());
    if (extraCol >= 0 && !d->m_extraHeaders.isEmpty()) {
        return extraColumnData(index.parent(), index.row(), extraCol, role);
    }
    return sourceModel()->data(mapToSource(index), role);
}

bool KExtraColumnsProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(const KExtraColumnsProxyModel);
    const int extraCol = extraColumnForProxyColumn(index.column());
    if (extraCol >= 0 && !d->m_extraHeaders.isEmpty()) {
        return setExtraColumnData(index.parent(), index.row(), extraCol, value, role);
    }
    return sourceModel()->setData(mapToSource(index), value, role);
}

Qt::ItemFlags KExtraColumnsProxyModel::flags(const QModelIndex &index) const
{
    const int extraCol = extraColumnForProxyColumn(index.column());
    if (extraCol >= 0) {
        // extra columns are readonly
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    return sourceModel() != nullptr ? sourceModel()->flags(mapToSource(index)) : Qt::NoItemFlags;
}

bool KExtraColumnsProxyModel::hasChildren(const QModelIndex &index) const
{
    if (index.column() > 0) {
        return false;
    }
    return QIdentityProxyModel::hasChildren(index);
}

QVariant KExtraColumnsProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_D(const KExtraColumnsProxyModel);
    if (orientation == Qt::Horizontal) {
        const int extraCol = extraColumnForProxyColumn(section);
        if (extraCol >= 0) {
            // Only text is supported, in headers for extra columns
            if (role == Qt::DisplayRole) {
                return d->m_extraHeaders.at(extraCol);
            }
            return QVariant();
        }
    }
    return QIdentityProxyModel::headerData(section, orientation, role);
}

QModelIndex KExtraColumnsProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    const int extraCol = extraColumnForProxyColumn(column);
    if (extraCol >= 0) {
        // We store the internal pointer of the index for column 0 in the proxy index for extra columns.
        // This will be useful in the parent method.
        return createIndex(row, column, QIdentityProxyModel::index(row, 0, parent).internalPointer());
    }
    return QIdentityProxyModel::index(row, column, parent);
}

QModelIndex KExtraColumnsProxyModel::parent(const QModelIndex &child) const
{
    const int extraCol = extraColumnForProxyColumn(child.column());
    if (extraCol >= 0) {
        // Create an index for column 0 and use that to get the parent.
        const QModelIndex proxySibling = createIndex(child.row(), 0, child.internalPointer());
        return QIdentityProxyModel::parent(proxySibling);
    }
    return QIdentityProxyModel::parent(child);
}

int KExtraColumnsProxyModel::extraColumnForProxyColumn(int proxyColumn) const
{
    if (sourceModel() != nullptr) {
        const int sourceColumnCount = sourceModel()->columnCount();
        if (proxyColumn >= sourceColumnCount) {
            return proxyColumn - sourceColumnCount;
        }
    }
    return -1;
}

int KExtraColumnsProxyModel::proxyColumnForExtraColumn(int extraColumn) const
{
    return sourceModel()->columnCount() + extraColumn;
}

void KExtraColumnsProxyModelPrivate::_ec_sourceLayoutAboutToBeChanged(const QList<QPersistentModelIndex> &sourceParents,
                                                                      QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(KExtraColumnsProxyModel);

    QList<QPersistentModelIndex> parents;
    parents.reserve(sourceParents.size());
    for (const QPersistentModelIndex &parent : sourceParents) {
        if (!parent.isValid()) {
            parents << QPersistentModelIndex();
            continue;
        }
        const QModelIndex mappedParent = q->mapFromSource(parent);
        Q_ASSERT(mappedParent.isValid());
        parents << mappedParent;
    }

    Q_EMIT q->layoutAboutToBeChanged(parents, hint);

    const QModelIndexList persistentIndexList = q->persistentIndexList();
    layoutChangePersistentIndexes.reserve(persistentIndexList.size());
    layoutChangeProxyColumns.reserve(persistentIndexList.size());

    for (QModelIndex proxyPersistentIndex : persistentIndexList) {
        proxyIndexes << proxyPersistentIndex;
        Q_ASSERT(proxyPersistentIndex.isValid());
        const int column = proxyPersistentIndex.column();
        layoutChangeProxyColumns << column;
        if (column >= q->sourceModel()->columnCount()) {
            proxyPersistentIndex = proxyPersistentIndex.sibling(proxyPersistentIndex.row(), 0);
        }
        const QPersistentModelIndex srcPersistentIndex = q->mapToSource(proxyPersistentIndex);
        Q_ASSERT(srcPersistentIndex.isValid());
        layoutChangePersistentIndexes << srcPersistentIndex;
    }
}

void KExtraColumnsProxyModelPrivate::_ec_sourceLayoutChanged(const QList<QPersistentModelIndex> &sourceParents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_Q(KExtraColumnsProxyModel);
    for (int i = 0; i < proxyIndexes.size(); ++i) {
        const QModelIndex proxyIdx = proxyIndexes.at(i);
        QModelIndex newProxyIdx = q->mapFromSource(layoutChangePersistentIndexes.at(i));
        if (proxyIdx.column() >= q->sourceModel()->columnCount()) {
            newProxyIdx = newProxyIdx.sibling(newProxyIdx.row(), layoutChangeProxyColumns.at(i));
        }
        q->changePersistentIndex(proxyIdx, newProxyIdx);
    }

    layoutChangePersistentIndexes.clear();
    layoutChangeProxyColumns.clear();
    proxyIndexes.clear();

    QList<QPersistentModelIndex> parents;
    parents.reserve(sourceParents.size());
    for (const QPersistentModelIndex &parent : sourceParents) {
        if (!parent.isValid()) {
            parents << QPersistentModelIndex();
            continue;
        }
        const QModelIndex mappedParent = q->mapFromSource(parent);
        Q_ASSERT(mappedParent.isValid());
        parents << mappedParent;
    }

    Q_EMIT q->layoutChanged(parents, hint);
}

#include "moc_KExtraColumnsProxyModel.cpp"
