/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KRecursiveFilterProxyModel.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
#include <QMetaMethod>

// Maintainability note:
// This class invokes some Q_PRIVATE_SLOTs in QSortFilterProxyModel which are
// private API and could be renamed or removed at any time.
// If they are renamed, the invocations can be updated with an #if (QT_VERSION(...))
// If they are removed, then layout{AboutToBe}Changed Q_SIGNALS should be used when the source model
// gets new rows or has rowsremoved or moved. The Q_PRIVATE_SLOT invocation is an optimization
// because layout{AboutToBe}Changed is expensive and causes the entire mapping of the tree in QSFPM
// to be cleared, even if only a part of it is dirty.
// Stephen Kelly, 30 April 2010.

// All this is temporary anyway, the long term solution is support in QSFPM: https://codereview.qt-project.org/151000

class KRecursiveFilterProxyModelPrivate
{
    Q_DECLARE_PUBLIC(KRecursiveFilterProxyModel)
    KRecursiveFilterProxyModel *q_ptr;

public:
    KRecursiveFilterProxyModelPrivate(KRecursiveFilterProxyModel *model)
        : q_ptr(model)
        , completeInsert(false)
    {
        qRegisterMetaType<QModelIndex>("QModelIndex");
    }

    inline QMetaMethod findMethod(const char *signature) const
    {
        Q_Q(const KRecursiveFilterProxyModel);
        const int idx = q->metaObject()->indexOfMethod(signature);
        Q_ASSERT(idx != -1);
        return q->metaObject()->method(idx);
    }

    // Convenience methods for invoking the QSFPM Q_SLOTS. Those slots must be invoked with invokeMethod
    // because they are Q_PRIVATE_SLOTs
    inline void invokeDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>())
    {
        Q_Q(KRecursiveFilterProxyModel);
        // required for Qt 5.5 and upwards, see commit f96baeb75fc in qtbase
        static const QMetaMethod m = findMethod("_q_sourceDataChanged(QModelIndex,QModelIndex,QVector<int>)");
        bool success = m.invoke(q, Qt::DirectConnection, Q_ARG(QModelIndex, topLeft), Q_ARG(QModelIndex, bottomRight), Q_ARG(QVector<int>, roles));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    inline void invokeRowsInserted(const QModelIndex &source_parent, int start, int end)
    {
        Q_Q(KRecursiveFilterProxyModel);
        static const QMetaMethod m = findMethod("_q_sourceRowsInserted(QModelIndex,int,int)");
        bool success = m.invoke(q, Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    inline void invokeRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
    {
        Q_Q(KRecursiveFilterProxyModel);
        static const QMetaMethod m = findMethod("_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)");
        bool success = m.invoke(q, Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    inline void invokeRowsRemoved(const QModelIndex &source_parent, int start, int end)
    {
        Q_Q(KRecursiveFilterProxyModel);
        static const QMetaMethod m = findMethod("_q_sourceRowsRemoved(QModelIndex,int,int)");
        bool success = m.invoke(q, Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    inline void invokeRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
    {
        Q_Q(KRecursiveFilterProxyModel);
        static const QMetaMethod m = findMethod("_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)");
        bool success = m.invoke(q, Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));
        Q_UNUSED(success);
        Q_ASSERT(success);
    }

    void sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right, const QVector<int> &roles = QVector<int>());
    void sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
    void sourceRowsInserted(const QModelIndex &source_parent, int start, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
    void sourceRowsRemoved(const QModelIndex &source_parent, int start, int end);

    /**
    Force QSortFilterProxyModel to re-evaluate whether to hide or show index and its parents.
    */
    void refreshAscendantMapping(const QModelIndex &index);

    QModelIndex lastFilteredOutAscendant(const QModelIndex &index);

    bool completeInsert;
    QModelIndex lastHiddenAscendantForInsert;
};

void KRecursiveFilterProxyModelPrivate::sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right, const QVector<int> &roles)
{
    QModelIndex source_parent = source_top_left.parent();
    Q_ASSERT(source_bottom_right.parent() == source_parent); // don't know how to handle different parents in this code...

    // Tell the world.
    invokeDataChanged(source_top_left, source_bottom_right, roles);

    // We can't find out if the change really matters to us or not, for a lack of a dataAboutToBeChanged signal (or a cache).
    // TODO: add a set of roles that we care for, so we can at least ignore the rest.

    // Even if we knew the visibility was just toggled, we also can't find out what
    // was the last filtered out ascendant (on show, like sourceRowsAboutToBeInserted does)
    // or the last to-be-filtered-out ascendant (on hide, like sourceRowsRemoved does)
    // So we have to refresh all parents.
    QModelIndex sourceParent = source_parent;
    while (sourceParent.isValid()) {
        invokeDataChanged(sourceParent, sourceParent, roles);
        sourceParent = sourceParent.parent();
    }
}

QModelIndex KRecursiveFilterProxyModelPrivate::lastFilteredOutAscendant(const QModelIndex &idx)
{
    Q_Q(KRecursiveFilterProxyModel);
    QModelIndex last = idx;
    QModelIndex index = idx.parent();
    while (index.isValid() && !q->filterAcceptsRow(index.row(), index.parent())) {
        last = index;
        index = index.parent();
    }
    return last;
}

void KRecursiveFilterProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end)
{
    Q_Q(KRecursiveFilterProxyModel);

    if (!source_parent.isValid() || q->filterAcceptsRow(source_parent.row(), source_parent.parent())) {
        // If the parent is already in the model (directly or indirectly), we can just pass on the signal.
        invokeRowsAboutToBeInserted(source_parent, start, end);
        completeInsert = true;
    } else {
        // OK, so parent is not in the model.
        // Maybe the grand parent neither.. Go up until the first one that is.
        lastHiddenAscendantForInsert = lastFilteredOutAscendant(source_parent);
    }
}

void KRecursiveFilterProxyModelPrivate::sourceRowsInserted(const QModelIndex &source_parent, int start, int end)
{
    Q_Q(KRecursiveFilterProxyModel);

    if (completeInsert) {
        // If the parent is already in the model, we can just pass on the signal.
        completeInsert = false;
        invokeRowsInserted(source_parent, start, end);
        return;
    }

    bool requireRow = false;
    for (int row = start; row <= end; ++row) {
        if (q->filterAcceptsRow(row, source_parent)) {
            requireRow = true;
            break;
        }
    }

    if (!requireRow) {
        // The new rows doesn't have any descendants that match the filter. Filter them out.
        return;
    }

    // Make QSFPM realize that lastHiddenAscendantForInsert should be shown now
    invokeDataChanged(lastHiddenAscendantForInsert, lastHiddenAscendantForInsert);
}

void KRecursiveFilterProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end)
{
    invokeRowsAboutToBeRemoved(source_parent, start, end);
}

void KRecursiveFilterProxyModelPrivate::sourceRowsRemoved(const QModelIndex &source_parent, int start, int end)
{
    Q_Q(KRecursiveFilterProxyModel);

    invokeRowsRemoved(source_parent, start, end);

    // Find out if removing this visible row means that some ascendant
    // row can now be hidden.
    // We go up until we find a row that should still be visible
    // and then make QSFPM re-evaluate the last one we saw before that, to hide it.

    QModelIndex toHide;
    QModelIndex sourceAscendant = source_parent;
    while (sourceAscendant.isValid()) {
        if (q->filterAcceptsRow(sourceAscendant.row(), sourceAscendant.parent())) {
            break;
        }
        toHide = sourceAscendant;
        sourceAscendant = sourceAscendant.parent();
    }
    if (toHide.isValid()) {
        invokeDataChanged(toHide, toHide);
    }
}

KRecursiveFilterProxyModel::KRecursiveFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new KRecursiveFilterProxyModelPrivate(this))
{
    setDynamicSortFilter(true);
}

KRecursiveFilterProxyModel::~KRecursiveFilterProxyModel() = default;

bool KRecursiveFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // TODO: Implement some caching so that if one match is found on the first pass, we can return early results
    // when the subtrees are checked by QSFPM.
    if (acceptRow(sourceRow, sourceParent)) {
        return true;
    }

    QModelIndex source_index = sourceModel()->index(sourceRow, 0, sourceParent);
    Q_ASSERT(source_index.isValid());
    bool accepted = false;

    const int numChildren = sourceModel()->rowCount(source_index);
    for (int row = 0, rows = numChildren; row < rows; ++row) {
        if (filterAcceptsRow(row, source_index)) {
            accepted = true;
            break;
        }
    }

    return accepted;
}

QModelIndexList KRecursiveFilterProxyModel::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    if (role < Qt::UserRole) {
        return QSortFilterProxyModel::match(start, role, value, hits, flags);
    }

    QModelIndexList list;
    if (!sourceModel()) {
        return list;
    }

    QModelIndex proxyIndex;
    const auto lst = sourceModel()->match(mapToSource(start), role, value, hits, flags);
    for (const QModelIndex &idx : lst) {
        proxyIndex = mapFromSource(idx);
        if (proxyIndex.isValid()) {
            list << proxyIndex;
        }
    }

    return list;
}

bool KRecursiveFilterProxyModel::acceptRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void KRecursiveFilterProxyModel::setSourceModel(QAbstractItemModel *model)
{
    // Standard disconnect of the previous source model, if present
    if (sourceModel()) {
        disconnect(sourceModel(),
                   SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)),
                   this,
                   SLOT(sourceDataChanged(QModelIndex, QModelIndex, QVector<int>)));

        disconnect(sourceModel(), SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)), this, SLOT(sourceRowsAboutToBeInserted(QModelIndex, int, int)));

        disconnect(sourceModel(), SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(sourceRowsInserted(QModelIndex, int, int)));

        disconnect(sourceModel(), SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)), this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex, int, int)));

        disconnect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(sourceRowsRemoved(QModelIndex, int, int)));
    }

    QSortFilterProxyModel::setSourceModel(model);

    // Disconnect in the QSortFilterProxyModel. These methods will be invoked manually
    // in invokeDataChanged, invokeRowsInserted etc.
    //
    // The reason for that is that when the source model adds new rows for example, the new rows
    // May not match the filter, but maybe their child items do match.
    //
    // Source model before insert:
    //
    // - A
    // - B
    // - - C
    // - - D
    // - - - E
    // - - - F
    // - - - G
    // - H
    // - I
    //
    // If the A F and L (which doesn't exist in the source model yet) match the filter
    // the proxy will be:
    //
    // - A
    // - B
    // - - D
    // - - - F
    //
    // New rows are inserted in the source model below H:
    //
    // - A
    // - B
    // - - C
    // - - D
    // - - - E
    // - - - F
    // - - - G
    // - H
    // - - J
    // - - K
    // - - - L
    // - I
    //
    // As L matches the filter, it should be part of the KRecursiveFilterProxyModel.
    //
    // - A
    // - B
    // - - D
    // - - - F
    // - H
    // - - K
    // - - - L
    //
    // when the QSortFilterProxyModel gets a notification about new rows in H, it only checks
    // J and K to see if they match, ignoring L, and therefore not adding it to the proxy.
    // To work around that, we make sure that the QSFPM slot which handles that change in
    // the source model (_q_sourceRowsAboutToBeInserted) does not get called directly.
    // Instead we connect the sourceModel signal to our own slot in *this (sourceRowsAboutToBeInserted)
    // Inside that method, the entire new subtree is queried (J, K *and* L) to see if there is a match,
    // then the relevant Q_SLOTS in QSFPM are invoked.
    // In the example above, we need to tell the QSFPM that H should be queried again to see if
    // it matches the filter. It did not before, because L did not exist before. Now it does. That is
    // achieved by telling the QSFPM that the data changed for H, which causes it to requery this class
    // to see if H matches the filter (which it now does as L now exists).
    // That is done in sourceRowsInserted.

    if (!model) {
        return;
    }

    disconnect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)), this, SLOT(_q_sourceDataChanged(QModelIndex, QModelIndex, QVector<int>)));

    disconnect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)), this, SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex, int, int)));

    disconnect(model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(_q_sourceRowsInserted(QModelIndex, int, int)));

    disconnect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)), this, SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex, int, int)));

    disconnect(model, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(_q_sourceRowsRemoved(QModelIndex, int, int)));

    // Slots for manual invoking of QSortFilterProxyModel methods.
    connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex, QVector<int>)), this, SLOT(sourceDataChanged(QModelIndex, QModelIndex, QVector<int>)));

    connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)), this, SLOT(sourceRowsAboutToBeInserted(QModelIndex, int, int)));

    connect(model, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(sourceRowsInserted(QModelIndex, int, int)));

    connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)), this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex, int, int)));

    connect(model, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(sourceRowsRemoved(QModelIndex, int, int)));
}

#include "moc_KRecursiveFilterProxyModel.cpp"

#endif
